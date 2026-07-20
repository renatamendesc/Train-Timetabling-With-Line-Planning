from data import Data
from model import ModelTrainTimetabling
from solution import Solution
from combinations import Combinations

import time
import copy
import threading
from itertools import product
from concurrent.futures import ThreadPoolExecutor
from itertools import permutations

class Heuristic:
    def __init__(self, data, nb_threads, time_limit_complete, time_limit_per_combination, solver):
        self.data = data

        self.nb_threads = nb_threads
        self.time_limit_complete = time_limit_complete
        self.time_limit_per_combination = time_limit_per_combination

        self.overall_best_sol = Solution()
        self.combinations = Combinations(data)

        self.improved_sol = None
        self.improved_sol_iter = None

        self.cyclical_routes_set = []
        self.initial_valid_routes_set = []

        self.baseline_combinations = []
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

    def calculate_max_trips_per_train(self, S, max_trips):
        
        n = len(max_trips)
        result = []
        seen = set()

        def is_valid(comb):
            return all(comb[i] <= max_trips[i] for i in range(n))

        def main_comb(comb):
            # generate all valid permutations
            perms_valids = [
                p for p in set(permutations(comb))
                if is_valid(p)
            ]
            return min(perms_valids) # main combination

        def backtrack(pos, remaining, combination):
            if pos == n:
                if remaining == 0:
                    comb = main_comb(tuple(combination))
                    if comb not in seen:
                        seen.add(comb)
                        result.append(comb)
                return

            for i in range(0, max_trips[pos] + 1):
                if i > remaining:
                    break
                backtrack(pos + 1, remaining - i, combination + [i])

        backtrack(0, S, [])
        
        return result

    def remove_trips_from_combinations(self, current_max_trips_per_train):
        new_candidates = []
        copy_baseline_combinations = copy.deepcopy(self.baseline_combinations)
        for candidate in copy_baseline_combinations:
            for t in range(self.data.nb_trains):
                for i in range(len(candidate[t])):
                    if i > current_max_trips_per_train[t]-1:
                        candidate[t][i] = self.data.nb_routes

            if self.combinations.verify_daily_demands(candidate) and self.combinations.normalize_combination(candidate):
                new_candidates.append(candidate)
        return new_candidates

    def try_combinations(self):

        self.improved_sol_iter = False
        for current_max_trips_per_train in self.current_max_trips_per_train_list:
            print(f"\nTesting combinations with {current_max_trips_per_train} trips:")
            self.candidate_combinations = self.remove_trips_from_combinations(current_max_trips_per_train)
            self.execute_candidate_combinations()

            if self.improved_sol:
                self.improved_sol_iter = True

    def create_short_turns(self):
        incomplete_routes = [i for i in range(self.data.nb_routes) if not self.data.is_cyclic_route(i)]

        new_routes = []
        for a in incomplete_routes:
            for b in incomplete_routes:
                if a == b:
                    continue

                # a must end where b starts, and b must end where a starts (so the union is cyclic)
                if self.data.are_incompatible_routes(a, b) or self.data.are_incompatible_routes(b, a):
                    continue

                a_vertices = self.data.route_vertices[a]
                b_vertices = self.data.route_vertices[b]

                # junction between a and b: same vertex, or a reversal at a depot
                if a_vertices[-1] == b_vertices[0]:
                    union = a_vertices + b_vertices[1:]
                elif self.data.is_depot[self.data.vertex_to_point[a_vertices[-1]]]:
                    union = a_vertices + b_vertices
                else:
                    continue

                # close the cycle back to the first vertex of a
                if union[-1] != a_vertices[0]:
                    if not self.data.is_depot[self.data.vertex_to_point[union[-1]]]:
                        continue
                    union = union + [a_vertices[0]]

                if union not in self.data.route_vertices and union not in new_routes:
                    new_routes.append(union)

        if not new_routes:
            return False

        # delete the incomplete routes, except the ones starting at the original depot
        kept_routes = []
        for i in range(self.data.nb_routes):
            if i not in incomplete_routes or self.data.vertex_to_point[self.data.route_vertices[i][0]] == self.data.initial_point:
                kept_routes.append(self.data.route_vertices[i])

        nb_removed = self.data.nb_routes - len(kept_routes)
        self.data.route_vertices = kept_routes + new_routes
        self.data.nb_routes = len(self.data.route_vertices)
        self.data.assign_arcs()

        print(f"\nCreated {len(new_routes)} short turn route(s), removed {nb_removed} incomplete route(s) - total routes: {self.data.nb_routes}")
        print(" - routes after modification...")
        for i in range(self.data.nb_routes):
            print(f"\troute #{i}: ", end="")
            for vertex in self.data.route_vertices[i]:
                print(f"{vertex}", end=" ")
            print()

        return True

    def execute_heuristic(self):

        self.start_time = time.time()
        self.time_limit_reached = False

        # unite non-complete routes to transforme into short turns
        self.create_short_turns()

        self.create_initial_candidates() # create baseline combinations
        for S in range(sum(self.data.max_trips_per_train), 0, -1):
            self.current_max_trips_per_train_list = self.calculate_max_trips_per_train(S, self.data.max_trips_per_train)
            self.try_combinations()
            if self.overall_best_sol.feasible and not self.improved_sol_iter:
                break

        # # if did not find any feasible solution, try again considering new routes 
        # if not self.overall_best_sol.feasible and self.try_new_set_of_routes():
        #     for S in range(sum(self.data.max_trips_per_train), 0, -1):
        #         self.current_max_trips_per_train_list = self.calculate_max_trips_per_train(S, self.data.max_trips_per_train)
        #         self.try_combinations()
        #         if self.overall_best_sol.feasible and not self.improved_sol_iter:
        #             break
        
        end_time = time.time()
        total_time = end_time - self.start_time
        
        if self.time_limit_reached:
            print("\n-> Time limit reached. Displaying best solution found so far...\n")
        else:
            print("\nFinished heuristic!\n")
        
        print(f"-> Total time = {total_time:.2f}", end="")

        self.overall_best_sol.rescale_values("heuristic")

        self.overall_best_sol.display_solution(self.data, "heuristic", self.time_limit_reached)
        self.overall_best_sol.save_solution(self.data, total_time, "heuristic", self.solver, self.time_limit_reached, self.nb_threads)
        self.overall_best_sol.create_graph(self.data, "heuristic",self.solver, self.nb_threads)

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
    def create_baseline_combinations(self):
        self.baseline_combinations.clear()
        nb_trains = self.data.nb_trains
        max_trips_per_train = self.data.max_trips_per_train
        max_nb_trips = self.data.max_nb_trips

        # for each train, we can have:
        # - an initial route
        # - a cyclic route (if there is more than 1 trip)
        all_initial_choices = product(self.initial_valid_routes_set, repeat=nb_trains)

        # if there is only 1 trip for all trains
        if max_nb_trips == 1:
            for initial_choice in all_initial_choices:
                combination = [[initial_choice[t]] for t in range(nb_trains)]

                if (self.combinations.verify_daily_demands(combination) and
                    self.combinations.normalize_combination(combination)):
                    self.baseline_combinations.append(combination)
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

                if valid and self.combinations.verify_daily_demands(combination):
                    self.baseline_combinations.append(combination)

    def create_initial_candidates(self):

        self.cyclical_routes_set.clear()
        self.initial_valid_routes_set.clear()

        self.create_cyclical_routes_set()
        self.create_initial_valid_routes_set()

        print(f"\nCreating initial set of candidate combinations...")

        self.create_baseline_combinations()

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
            self.create_baseline_combinations()
            return True

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
            
        # print(f"Solving combination: {candidate_combination}")
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
                    print(f"-> New best combination: {self.overall_best_sol.routes_combination} - Objective value: {self.overall_best_sol.obj_value}")
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
            print(f"{self.counter_solved}/{len(self.candidate_combinations)} candidate combination(s) tested! - Feasible: {feasible}")

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
