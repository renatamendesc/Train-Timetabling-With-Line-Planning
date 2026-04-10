import numpy as np
import math
from threading import Lock

ULLONG_MAX = (1 << 64) - 1

class Combinations:
    def __init__(self, data):
        self.data = data

        self.all_trips_combinations = None
        self.unique_combinations = set()
        self.lock = Lock()

    def verify_sequence_feasibility(self, seq, route_not_completed):
        flag = True
        last_idx = len(seq) - 1
        for i in range(last_idx):
            cur = seq[i]
            nxt = seq[i + 1]

            if cur != route_not_completed:
                # verify compatibility between consecutive routes
                if nxt != route_not_completed:
                    if self.data.are_incompatible_routes(cur, nxt):
                        return False
            else:
                # after a non-trip, no trips can exist
                if nxt != route_not_completed:
                    return False

        return flag
        
    def check_trips_feasibility(self, seq):

        # seq: represents a sequence of trips a single train can complete
        route_not_completed = self.data.nb_routes

        # verify whether first route starts at the initial depot
        if seq[0] != route_not_completed:
            if not self.data.is_valid_route(0, 0, seq[0]):
                return False
        else:
            # verify whether other routes are also not completed
            if not all(route == route_not_completed for route in seq[1:]):
                return False
            else:
                return True

        return self.verify_sequence_feasibility(seq, route_not_completed)

    def generate_trips_combinations(self, total, max_trips):

        current = np.zeros(max_trips, dtype=np.uint16)

        valid_list = []
        routes_plus_1 = self.data.nb_routes + 1
        for _ in range(total):
            if self.check_trips_feasibility(current):
                valid_list.append(current.copy())

            # go to next combination
            for pos in range(max_trips - 1, -1, -1):
                current[pos] += 1
                if current[pos] < routes_plus_1:
                    break
                current[pos] = 0

        return valid_list

    def calculate_trips_combinations(self):

        def verify_overflow(base, exp):
            if base <= 1:
                return False
            return exp * math.log(base) > math.log(ULLONG_MAX)

        self.all_trips_combinations = [None] * self.data.nb_trains
        computed = {}

        for i in range(self.data.nb_trains):
            max_trips = self.data.max_trips_per_train[i]

            if max_trips in computed:
                self.all_trips_combinations[i] = computed[max_trips]
                continue

            if verify_overflow(self.data.nb_routes + 1, max_trips):
                raise ValueError("Too many combiantions! Aborting...")

            total = (self.data.nb_routes + 1) ** max_trips
            self.all_trips_combinations[i] = self.generate_trips_combinations(total,max_trips)
            computed[max_trips] = self.all_trips_combinations[i]

    def normalize_combination(self, combination):
        nb_routes = self.data.nb_routes

        # ensure all dimensions have the same size
        max_trips = self.data.max_nb_trips
        for i in range(len(combination)):
            # convert numpy array to list if needed
            if isinstance(combination[i], np.ndarray):
                combination[i] = combination[i].tolist()
            while len(combination[i]) < max_trips:
                combination[i].append(nb_routes)

        # create a normalized copy
        normalized_combination = sorted((tuple(row) for row in combination))
        normalized_tuple = tuple(normalized_combination)

        # insert with thread safety
        with self.lock:
            if normalized_tuple in self.unique_combinations:
                return False  # already seen
            self.unique_combinations.add(normalized_tuple)
        return True  # a new combination

    def verify_daily_demands(self, combination):

        times_vertex_was_visited = [0] * self.data.get_nb_vertices()
        for i in range(len(combination)):
            for j in range(len(combination[i])):
                if combination[i][j] == -1: # if route is undefined cannot calculate if demands were met
                    return True
                if combination[i][j] != self.data.nb_routes:
                    route = combination[i][j]
                    for vertex in self.data.route_vertices[route]:
                        times_vertex_was_visited[vertex] += 1

        # if any demand was not met, invalid combination
        for i in range(self.data.get_nb_vertices()):
            if times_vertex_was_visited[i] < self.data.demand_per_day[i]:
                return False

        return True

    def is_valid_combination(self, combination):

        if not self.normalize_combination(combination):
            return False

        if not self.verify_daily_demands(combination):
            return False

        return True



