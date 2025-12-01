from data import Data
from model import Model
from solution import Solution
from combinations import Combinations

class Heuristic:
    def __init__(self, data, nb_threads, time_limit_complete, time_limit_per_combination):
        self.data = data

        self.nb_threads = nb_threads
        self.time_limit_complete = time_limit_complete
        self.time_limit_per_combination = time_limit_per_combination

        self.overall_best_sol = Solution()
        self.combinations = Combinations(data)

        self.improved_sol = False

        self.cyclical_routes_set = []
        self.initial_valid_routes_set = []

        self.candidate_combinations = []

    def create_cyclical_routes_set(self):

        have_full_cycles = False
        have_cycles = False
        for i in range(self.data.nb_routes):
            if len(self.data.get_route_vertices(i)) == self.data.nb_vertices + 1 and self.data.is_cyclic_route(i):
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

    def create_initial_candidates(self):
        self.create_cyclical_routes_set()
        self.create_initial_valid_routes_set()

        print(f"Creating initial set of candidate combinations...")

        # create maximum size combinations
        nb_trains = self.data.nb_trains
        num_valid = len(self.initial_valid_routes_set)
        num_cyclic = len(self.cyclical_routes_set)

        # map initial routes done by each train
        initial_indices = [0] * nb_trains
        done_initial = False
        while not done_initial:
            # map cyclical routes done by each train
            cyclic_indices = [0] * nb_trains
            done_cyclic = False
            while not done_cyclic:
                combination = []
                valid_combination = True

                for t in range(nb_trains):
                    nb_trips = self.data.max_trips_per_train[t]
                    routes_of_train = [None] * nb_trips

                    # choose initial trip
                    routes_of_train[0] = self.initial_valid_routes_set[initial_indices[t]]

                    # choose cyclical routes
                    for k in range(1, nb_trips):
                        routes_of_train[k] = self.cyclical_routes_set[cyclic_indices[t]]

                    # verify whether sequence of routes is compatible
                    if not self.combinations.check_sequence_feasibility(routes_of_train):
 





        #     # map initial routes done by each train
        #     initial_indices = [0] * nb_trains
        #     done_initial = False

        #     while not done_initial:
        #         # map cyclical routes done by each train
        #         cyclic_indices = [0] * nb_trains
        #         done_cyclic = False

        #         while not done_cyclic:
        #             combination = []
        #             valid_combination = True

        #             for t in range(nb_trains):
        #                 nb_trips = data.get_train_max_trips(t)
        #                 routes_of_train = [None] * nb_trips

        #                 # choose initial trip
        #                 routes_of_train[0] = initial_valid_routes_set[initial_indices[t]]

        #                 # choose cyclical route
        #                 for k in range(1, nb_trips):
        #                     routes_of_train[k] = cyclical_routes_set[cyclic_indices[t]]

        #                 # verify whether sequence of routes is compatible
        #                 if verify_compatibility(data, routes_of_train):
        #                     combination.append(routes_of_train)
        #                 else:
        #                     valid_combination = False

        #             # verify whether demands are met
        #             if (valid_combination and
        #                 verify_demands(data, combination) and
        #                 normalize_candidates(data, combination)):
        #                 candidate_combinations.append(combination)

        #             # stop if only one trip per train
        #             if data.get_max_nb_trips() == 1:
        #                 break

        #             # increment cyclical indices (base-#num_cyclic counter)
        #             for i in reversed(range(nb_trains)):
        #                 cyclic_indices[i] += 1
        #                 if cyclic_indices[i] < num_cyclic:
        #                     break
        #                 else:
        #                     cyclic_indices[i] = 0
        #                     if i == 0:
        #                         done_cyclic = True

        #         # increment initial indices (base-#num_valid counter)
        #         for i in reversed(range(nb_trains)):
        #             initial_indices[i] += 1
        #             if initial_indices[i] < num_valid:
        #                 break
        #             else:
        #                 initial_indices[i] = 0
        #                 if i == 0:
        #                     done_initial = True

        #     return candidate_combinations

    def solve_initial_candidates(self):
        pass

    def create_subsets_from_best_combination(self):
        pass
    def create_subsets_from_all_combinations(self):
        pass

    def execute_candidate_combinations(self):
        pass

    def execute_heuristic(self):

        start_time = time.time()

        iter = 0
        self.create_initial_candidates()
        self.solve_initial_candidates()
        while self.improved_sol:
            iter += 1

            # verify time limit

            # create subsets of candidate combinations
            create_subsets_from_best_combination()

            execute_candidate_combinations()

        # unfix final trips
        unfix_trips_from_best_combination()

        end_time = time.time()
        total_time = end_time - start_time
        print(f"-> Total time = {total_time:.2f}", end="")
