from model import ModelTrainTimetabling
from solution import Solution
from combinations import Combinations

import time
import copy
import threading
from itertools import product, permutations, combinations_with_replacement
from concurrent.futures import ThreadPoolExecutor

class Heuristic:
    def __init__(self, data, nb_threads, time_limit_complete, time_limit_per_combination, solver):

        # initialize parameters
        self.data = data
        self.nb_threads = nb_threads
        self.time_limit_complete = time_limit_complete
        self.time_limit_per_combination = time_limit_per_combination
        self.solver = solver

        self.overall_best_sol = Solution()     # store the current best solution
        self.combinations = Combinations(data) # store the combinations to be explored

        self.full_cycle_routes = []            # store routes with full cycles (passes by all points of the railway)
        self.short_turn_routes = []            # store routes with short turns

        self.baseline_combinations = []        # baseline combinations, used to create reduced combinations with ledd trips
        self.candidate_combinations = []       # current candidate combinations

        self.route_min_time = None             # minimum duration of each route (used to calculate lower bound for combinations)

        self.time_limit_reached = False        # flag to tell if the time limit was reached
        self.start_time = None                 # store start time

        # variables used to administrate execution with multiple threads
        self.thread_local = threading.local()
        self.best_lock = threading.Lock()
        self.progress_lock = threading.Lock()
        self.time_limit_lock = threading.Lock()

# ===============================================================================================================
# =================================== functions used to calculate lower bound ===================================
# ===============================================================================================================        

    def compute_route_min_times(self):
        # minimum duration of each route: sum of minimum travel + service times of its arcs
        self.route_min_time = [sum(self.data.distance_and_service_min[arc["idx"]] for arc in self.data.route_arcs[r]) for r in range(self.data.nb_routes)]

    def combination_score(self, combination):
        # lower bound on the makespan: trains run in parallel, so the busiest train dominates
        train_times = [sum(self.route_min_time[r] for r in routes if 0 <= r < self.data.nb_routes) for routes in combination]
        return (max(train_times), sum(train_times))

# ===============================================================================================================
# ===============================================================================================================

# ===============================================================================================================
# ============================= functions used to create combinations according =================================
# ============================= to the number of trips                          =================================
# ===============================================================================================================

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
            # verify if combination is still valid
            if self.combinations.verify_daily_demands(candidate) and self.combinations.normalize_combination(candidate):
                new_candidates.append(candidate)
        return new_candidates

# ===============================================================================================================
# ===============================================================================================================

# ===============================================================================================================
# ============================= functions used to create the baseline combinations ==============================
# ===============================================================================================================

    def classify_routes(self):
        # split the cyclic routes in full cycles and short turns
        full_length = self.data.get_nb_vertices() + 1
        self.full_cycle_routes = []
        self.short_turn_routes = []
        for r in range(self.data.nb_routes):
            if not self.data.is_cyclic_route(r):
                continue
            if len(self.data.route_vertices[r]) == full_length:
                self.full_cycle_routes.append(r)
            else:
                self.short_turn_routes.append(r)

        # without full cycles, the short turns are the only cyclic bodies available
        if not self.full_cycle_routes:
            self.full_cycle_routes = list(self.short_turn_routes)

        if not self.full_cycle_routes:
            print("Instance does not have any cyclical routes...")

    def is_incomplete_route(self, r):
        # incomplete route: non-cyclic and does not pass through every point of the line
        if self.data.is_cyclic_route(r):
            return False
        points_covered = {self.data.vertex_to_point[v] for v in self.data.route_vertices[r]}
        return len(points_covered) < self.data.nb_points

    def create_pair_bodies(self):
        # two-trip short turn patterns: ordered pairs of incomplete routes where
        # each one ends at the point the other starts, closing a cycle in two trips
        pairs = []
        for a in range(self.data.nb_routes):
            if not self.is_incomplete_route(a):
                continue
            for b in range(self.data.nb_routes):
                if a == b or not self.is_incomplete_route(b):
                    continue
                if self.data.are_incompatible_routes(a, b) or self.data.are_incompatible_routes(b, a):
                    continue
                pairs.append((a, b))
        return pairs

    def create_train_sequences(self, nb_trips):
        # all valid trip sequences for a train with nb_trips trips, in two shapes:
        # an initial route followed by a repeated body, where the body is a cyclic route
        # (full cycle or short turn) or a pair of non-cyclic routes closing a short-turn
        # in two trips; and a short-turn pair opening the day, then a repeated full cycle

        initial_routes = [r for r in range(self.data.nb_routes) if self.data.is_valid_route(0, 0, r)]
        single_bodies = list(dict.fromkeys(self.full_cycle_routes + self.short_turn_routes))
        pair_bodies = self.create_pair_bodies()

        candidates = []
        for initial_route in initial_routes:
            for body in single_bodies:
                candidates.append([initial_route] + [body] * (nb_trips - 1))
            for a, b in pair_bodies:
                pattern = (a, b)
                candidates.append([initial_route] + [pattern[i % 2] for i in range(nb_trips - 1)])

        # mixed bodies -> [a, b, c, c, ...]: a short-turn pair opening the day, then a
        # repeated full cycle. Captures optima that open with a short turn and then switch
        # to full cycles (e.g. [2,3,4,4]), which a single repeated body cannot express,
        # without the blowup of enumerating arbitrary route chains
        if nb_trips >= 3:  # need the pair (2 trips) plus at least one full cycle
            for a, b in pair_bodies:
                # the pair opener must be a valid first trip (starts at the initial point)
                if not self.data.is_valid_route(0, 0, a):
                    continue
                for c in self.full_cycle_routes:
                    candidates.append([a, b] + [c] * (nb_trips - 2))

        sequences = []
        seen = set()
        for routes in candidates:
            key = tuple(routes)
            if key in seen:
                continue
            seen.add(key)

            # verify feasibility of the sequence by train
            if self.combinations.verify_sequence_feasibility(routes, self.data.nb_routes+1):
                sequences.append(routes)

        return sequences

    def create_baseline_combinations(self):
        nb_trains = self.data.nb_trains
        max_trips_per_train = self.data.max_trips_per_train
        max_nb_trips = self.data.max_nb_trips

        # if there is only 1 trip for all trains
        if max_nb_trips == 1:
            initial_routes = [r for r in range(self.data.nb_routes) if self.data.is_valid_route(0, 0, r)] # use all valid routes for the first trip (starts at the initial vertex)
            all_initial_choices = product(initial_routes, repeat=nb_trains)                               # get all combinations of initial trips for trains
            for initial_choice in all_initial_choices:
                combination = [[initial_choice[t]] for t in range(nb_trains)]

                # verify if it is a valid combination
                if (self.combinations.verify_daily_demands(combination) and self.combinations.normalize_combination(combination)):
                    self.baseline_combinations.append(combination)
            return

        self.classify_routes() # create sets of full cycles and short turns

        # create combiantions
        groups = {}
        for t in range(nb_trains):
            groups.setdefault(max_trips_per_train[t], []).append(t)                                                               # group trains according to number of trips
        group_keys = sorted(groups.keys())
        sequences_per_group = {k: self.create_train_sequences(k) for k in group_keys}                                             # create sequences of trips for each group
        all_group_choices = product(*[combinations_with_replacement(sequences_per_group[k], len(groups[k])) for k in group_keys]) # create the combinations where the order is not important 
        for group_choice in all_group_choices:
            combination = [None] * nb_trains
            for k, chosen_sequences in zip(group_keys, group_choice):
                for t, routes in zip(groups[k], chosen_sequences):
                    combination[t] = list(routes)
            # verify if it is a valid combination
            if self.combinations.verify_daily_demands(combination):
                self.baseline_combinations.append(combination)

# ===============================================================================================================
# ===============================================================================================================

# ===============================================================================================================
# ================================== functions used to solve the combinations ===================================
# ===============================================================================================================

    def solve_combination_task(self, candidate_combination):

        # verify time limit before starting to solve
        elapsed = time.time() - self.start_time
        if self.time_limit_reached or elapsed > self.time_limit_complete:
            with self.time_limit_lock:
                self.time_limit_reached = True
            with self.progress_lock:
                self.counter_solved += 1
            return

        # prune by lower bound: if the busiest train cannot beat the current best, skip the solver
        if self.combination_score(candidate_combination)[0] >= self.overall_best_sol.obj_value:
            with self.progress_lock:
                self.counter_solved += 1
                print(f"{self.counter_solved}/{len(self.candidate_combinations)} candidate combination(s) tested! - Pruned by lower bound")
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
                    print(f"-> New best combination: {self.overall_best_sol.routes_combination} - Objective value: {self.overall_best_sol.obj_value}")
                # elif model_thread.best_solution.obj_value == self.overall_best_sol.obj_value:
                #     if model_thread.best_solution.max_nb_repeated_routes > self.overall_best_sol.max_nb_repeated_routes:
                #         self.overall_best_sol = copy.deepcopy(model_thread.best_solution)

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

# ===============================================================================================================
# ===============================================================================================================

# ===============================================================================================================
# ===============================================================================================================

    def run_exploration_loop(self):
        
        for S in range(1, sum(self.data.max_trips_per_train)+1, 1): # for each total ammount of trips 
            self.current_max_trips_per_train_list = self.calculate_max_trips_per_train(S, self.data.max_trips_per_train)
            for current_max_trips_per_train in self.current_max_trips_per_train_list:
                self.candidate_combinations = self.remove_trips_from_combinations(current_max_trips_per_train) # remove trips from baseline combinations
                self.candidate_combinations.sort(key=self.combination_score)                                   # solve the most promising candidates first so the cutoff tightens early
                if len(self.candidate_combinations) > 0:
                    print(f"\nTesting combinations with {current_max_trips_per_train} trips:")

                self.execute_candidate_combinations()                                                          # solve combinations

            if self.time_limit_reached:
                break

    def execute_heuristic(self):

        self.start_time = time.time()    # start counting execution time
        self.time_limit_reached = False

        self.compute_route_min_times()   # calculate minimum time of each route

        print(f"Creating baseline set of candidate combinations...")
        self.create_baseline_combinations()
        print(f"Starting exploratino loop...")
        self.run_exploration_loop()

        end_time = time.time()
        total_time = end_time - self.start_time

        # verify if time limit was reach
        self.time_limit_reached = (total_time >= self.time_limit_complete)
        if self.time_limit_reached:
            print("\n-> Time limit reached. Displaying best solution found so far...\n")
        else:
            print("\nFinished heuristic!\n")

        print(f"-> Total time = {total_time:.2f}", end="")
        self.overall_best_sol.rescale_values("heuristic")
        self.overall_best_sol.display_solution(self.data, "heuristic", self.time_limit_reached)
        self.overall_best_sol.save_solution(self.data, total_time, "heuristic", self.solver, self.time_limit_reached, self.nb_threads)
        self.overall_best_sol.create_graph(self.data, "heuristic",self.solver, self.nb_threads)

# ===============================================================================================================
# ===============================================================================================================