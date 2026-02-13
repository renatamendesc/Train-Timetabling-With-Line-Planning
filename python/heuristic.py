from data import Data
from model import ModelTrainTimetabling
from solution import Solution
from combinations import Combinations

import time
import copy
import threading
from itertools import product
from concurrent.futures import ThreadPoolExecutor

class Heuristic:
    def __init__(self, data, nb_threads, time_limit_complete, time_limit_per_combination, solver):
        self.data = data

        self.nb_threads = nb_threads
        self.time_limit_complete = time_limit_complete
        self.time_limit_per_combination = time_limit_per_combination

        self.overall_best_sol = Solution()
        self.combinations = Combinations(data)

        self.improved_sol = None

        self.cyclical_routes_set = []
        self.initial_valid_routes_set = []

        self.candidate_combinations = []

        self.iter = 0

        # thread local variables
        self.thread_local = threading.local()

        # locks
        self.best_lock = threading.Lock()
        self.progress_lock = threading.Lock()
        self.time_limit_lock = threading.Lock()
        
        # flag to tell if the time limit was reached
        self.time_limit_reached = False
        self.start_time = None

        self.solver = solver

    def create_cyclical_routes_set(self):
        have_full_cycles = False
        have_cycles = False
        for i in range(self.data.nb_routes):
            if len(self.data.route_vertices[i]) == self.data.get_nb_vertices() + 1 and self.data.is_cyclic_route(i):
                self.cyclical_routes_set.append(i)
                have_full_cycles = True
                have_cycles = True

        if not have_full_cycles:
            for i in range(self.data.nb_routes):
                if self.data.is_cyclic_route(i):
                    self.cyclical_routes_set.append(i)
                    have_cycles = True

        if not have_cycles:
            print("Instance does not have any cyclical routes...")
    def create_initial_valid_routes_set(self):

        to_be_removed = [True] * len(self.cyclical_routes_set) # false for not being removed, true for being removed
        
        for i in range(self.data.nb_routes):
            if self.data.is_valid_route(0, 0, i):
                found_compatible = False
                # verify whether initial route is compatible with any cyclical route before adding to set
                for j in range(len(self.cyclical_routes_set)):
                    if not self.data.are_incompatible_routes(i, self.cyclical_routes_set[j]):
                        found_compatible = True
                        to_be_removed[j] = False
                if found_compatible:
                    self.initial_valid_routes_set.append(i)

        for i in range(len(to_be_removed)):
            if to_be_removed[i] == True:
                self.cyclical_routes_set.pop(i)
    def create_maximum_size_candidates(self):
        self.candidate_combinations.clear()
        nb_trains = self.data.nb_trains
        max_trips_per_train = self.data.max_trips_per_train
        max_nb_trips = self.data.max_nb_trips

        # for each train, we can have:
        # - an initial route
        # - a cyclic route (if there are more than 1 trip)
        all_initial_choices = product(self.initial_valid_routes_set, repeat=nb_trains)

        # if there is only 1 trip for all trains
        if max_nb_trips == 1:
            for initial_choice in all_initial_choices:
                combination = [[initial_choice[t]] for t in range(nb_trains)]

                if (self.combinations.verify_daily_demands(combination) and
                    self.combinations.normalize_combination(combination)):
                    self.candidate_combinations.append(combination)
            return

        # if there are multiple trips
        for initial_choice in all_initial_choices:
            all_cyclic_choices = product(self.cyclical_routes_set, repeat=nb_trains)
            for cyclic_choice in all_cyclic_choices:
                combination = []
                valid = True

                for t in range(nb_trains):
                    nb_trips = max_trips_per_train[t]

                    # initial route + repeated cyclic route
                    routes = [initial_choice[t]] + [cyclic_choice[t]] * (nb_trips - 1)

                    # verify feasibility of the sequence by train
                    if not self.combinations.verify_sequence_feasibility(routes, self.data.nb_routes+1):
                        valid = False
                        break

                    combination.append(routes)

                if valid and self.combinations.verify_daily_demands(combination) and self.combinations.normalize_combination(combination):
                    self.candidate_combinations.append(combination)

    def create_initial_candidates(self):
        self.create_cyclical_routes_set()
        self.create_initial_valid_routes_set()

        print(f"Creating initial set of candidate combinations...")

        self.create_maximum_size_candidates()

    def try_new_set_of_routes(self):
        old_initial_valid_routes_set = copy.deepcopy(self.initial_valid_routes_set)
        old_cyclical_routes_set = copy.deepcopy(self.cyclical_routes_set)

        self.cyclical_routes_set.clear()
        self.initial_valid_routes_set.clear()

        # all cyclic routes included
        for i in range(self.data.nb_routes):
            if self.data.is_cyclic_route(i):
                self.cyclical_routes_set.append(i)
        
        self.create_initial_valid_routes_set()

        if old_initial_valid_routes_set == self.initial_valid_routes_set and old_cyclical_routes_set == self.cyclical_routes_set:
            return False
        else:
            self.create_maximum_size_candidates()
            return True

    # def unfix_trips_from_best_combination(self):
    #     routes = self.overall_best_sol.routes_combination
    #     nb_trains = self.data.nb_trains

    #     for i in range(nb_trains):
    #         if not routes[i]: 
    #             routes[i].append(-1)
    #         else:
    #             routes[i][-1] = -1

    #     self.model = ModelTrainTimetabling(self.data, self.nb_threads, self.time_limit_complete, self.time_limit_per_combination, self.solver)
    #     self.model.initialize()
    #     self.model.create_model_for_combination(routes)
    #     self.model.execute_solver_for_combination("heuristic", self.overall_best_sol.obj_value)

    def unfix_trips_from_all_combinations(self):
        new_candidates = []
        nb_trains = self.data.nb_trains
        for tested in self.candidate_combinations:
            candidate = [trips[:] for trips in tested]

            for i in range(nb_trains):
                if not candidate[i]:
                    candidate[i].append(-1)
                else:
                    candidate[i][-1] = -1

            if self.combinations.normalize_combination(candidate):
                new_candidates.append(candidate)
        self.candidate_combinations = new_candidates

    def remove_trips(self, current_combination, max_nb_trips):
        nb_trains = self.data.nb_trains
        total_combinations = 1 << nb_trains # 2^n possibilidades
        new_candidates = []

        for mask in range(total_combinations):
            if mask == 0:
                continue

            new_combination = []
            valid = True

            for i in range(nb_trains):
                trips = current_combination[i]

                if (mask >> i) & 1: # tries to remove final trip from train
                    if len(trips) < max_nb_trips:
                        valid = False
                        break

                    new_combination.append(trips[:-1])
                else:
                    new_combination.append(list(trips))

            if not valid:
                continue

            # print("New combination:", new_combination)

            if (self.combinations.verify_daily_demands(new_combination) and
                self.combinations.normalize_combination(new_combination)):
                new_candidates.append(new_combination)

        return new_candidates
    def create_subsets_from_best_combination(self):
        max_trips = self.data.max_nb_trips - self.iter

        new_candidates = self.remove_trips(self.overall_best_sol.routes_combination, max_trips)

        if not new_candidates:
            return False

        self.candidate_combinations = new_candidates
        return True
    def create_subsets_from_all_combinations(self):
        max_trips = self.data.max_nb_trips - self.iter
        print("Novo maximo de viagens por trem:", max_trips)
        new_candidates = []

        for candidate in self.candidate_combinations:
            new_candidates.extend(self.remove_trips(candidate, max_trips))

        if not new_candidates:
            return False

        self.candidate_combinations = new_candidates
        return True

    def fix_one_train_not_completing_any_trip(self):
        nb_trains = self.data.nb_trains
        self.original_candidates = self.candidate_combinations[:]
        self.candidate_combinations.clear() 
        for candidate in self.original_candidates:
            print(f"Candidate: {candidate}")
            for i in range(nb_trains):
                new_candidate = copy.deepcopy(candidate)
                for j in range(len(candidate[i])):
                    new_candidate[i][j] = self.data.nb_routes
                print(f"Candidate after fixing train {i}: {new_candidate}")
                if self.combinations.verify_daily_demands(new_candidate) and self.combinations.normalize_combination(new_candidate):
                    self.candidate_combinations.append(new_candidate)

    def solve_initial_candidates(self):
        self.execute_candidate_combinations()

        if not self.improved_sol and not self.time_limit_reached:
            # create subsets from all candidates combinations
            print("\nCreating subsets from all candidates combinations...")
            if self.create_subsets_from_all_combinations():
                self.execute_candidate_combinations()
                
            if not self.improved_sol and not self.time_limit_reached:
                # update set of valid routes
                print("\nUpdating set of valid routes...")
                if self.try_new_set_of_routes():
                    self.execute_candidate_combinations()

            if not self.improved_sol and not self.time_limit_reached:
                # allow model to choose the last trips completed by the trains
                print("\nUnfixing trips from all combinations...")
                self.unfix_trips_from_all_combinations()
                self.execute_candidate_combinations()

            if not self.improved_sol and not self.time_limit_reached:
                # fix one train not completing any trip
                print("\nFixing one train not completing any trip...")
                self.fix_one_train_not_completing_any_trip()
                self.execute_candidate_combinations()

    def solve_combination_task(self, candidate_combination):

        # verify time limit before starting to solve
        elapsed = time.time() - self.start_time
        if self.time_limit_reached or elapsed > self.time_limit_complete:
            with self.time_limit_lock:
                self.time_limit_reached = True
            with self.progress_lock:
                self.counter_solved += 1
            return

        if not hasattr(self.thread_local, "model"):
            self.thread_local.model = ModelTrainTimetabling(self.data, self.nb_threads, self.time_limit_complete, self.time_limit_per_combination, self.solver)
            self.thread_local.model.initialize()

        model_thread = self.thread_local.model

        print(f"Solving combination: {candidate_combination}")
        model_thread.reset()
        model_thread.create_model_for_combination(candidate_combination)
        
        # verify time limit before executing the solver
        if self.time_limit_reached or (time.time() - self.start_time > self.time_limit_complete):
            with self.time_limit_lock:
                self.time_limit_reached = True
            with self.progress_lock:
                self.counter_solved += 1
            return
        
        feasible = model_thread.execute_solver_for_combination("heuristic", self.overall_best_sol.obj_value)
        if feasible:
            with self.best_lock:
                if model_thread.best_solution.obj_value < self.overall_best_sol.obj_value:
                    self.overall_best_sol = copy.deepcopy(model_thread.best_solution)
                    self.improved_sol = True
                elif model_thread.best_solution.obj_value == self.overall_best_sol.obj_value:
                    if model_thread.best_solution.max_nb_repeated_routes > self.overall_best_sol.max_nb_repeated_routes:
                        self.overall_best_sol = copy.deepcopy(model_thread.best_solution)
                        self.improved_sol = True

        # verify time limit before executing the solver
        if self.time_limit_reached or (time.time() - self.start_time > self.time_limit_complete):
            with self.time_limit_lock:
                self.time_limit_reached = True
            with self.progress_lock:
                self.counter_solved += 1
            return

        with self.progress_lock:
            self.counter_solved += 1
            print(f"{self.counter_solved}/{len(self.candidate_combinations)} candidate combination(s) tested! - Feasible: {feasible} - Objective value: {model_thread.best_solution.obj_value}")

    def execute_candidate_combinations(self):

        self.improved_sol = False
        self.counter_solved = 0
        
        futures = []
        with ThreadPoolExecutor(max_workers=self.nb_threads) as executor:
            for count in range(len(self.candidate_combinations)):
                # verify time limit before submitting new task
                if self.start_time is not None:
                    if self.time_limit_reached or (time.time() - self.start_time > self.time_limit_complete):
                        with self.time_limit_lock:
                            self.time_limit_reached = True
                        break
                
                future = executor.submit(self.solve_combination_task, self.candidate_combinations[count])
                futures.append(future)

            # if time limit was reached, try to cancel pending tasks
            if self.time_limit_reached:
                cancelled = 0
                for future in futures:
                    if future.cancel():
                        cancelled += 1
                if cancelled > 0:
                    print(f"Cancelled {cancelled} pending tasks. Waiting for running tasks to finish...")
            
            # wait for the tasks to finish
            executor.shutdown(wait=True)

    def execute_heuristic(self):

        self.start_time = time.time()
        self.time_limit_reached = False

        self.iter = 0
        self.create_initial_candidates()
        self.solve_initial_candidates()
        
        # print(f"found feasible solution? {self.improved_sol}")
        while self.improved_sol and not self.time_limit_reached:
            self.iter += 1

            # verify time limit
            if time.time() - self.start_time >= self.time_limit_complete:
                with self.time_limit_lock:
                    self.time_limit_reached = True
                print(f"\n-> Time limit reached ({self.time_limit_complete:.2f}s). Stopping algorithm.")
                break

            # create subsets of candidate combinations
            if not self.create_subsets_from_best_combination():
                break

            self.execute_candidate_combinations()

        end_time = time.time()
        total_time = end_time - self.start_time
        
        if self.time_limit_reached:
            print("\n-> Time limit reached. Displaying best solution found so far...\n")
        else:
            print("\nFinished heuristic!\n")
        
        print(f"-> Total time = {total_time:.2f}", end="")
        self.overall_best_sol.display_solution(self.data, "heuristic", self.time_limit_reached)
        self.overall_best_sol.save_solution(self.data, total_time, "heuristic", self.time_limit_reached, self.nb_threads)
