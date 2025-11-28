import numpy as np
import math

ULLONG_MAX = (1 << 64) - 1

class Combinations:
    def __init__(self, data):
        self.data = data

        self.all_trips_combinations = None

    def check_trips_feasibility(self, seq):

        # seq: represents a sequence of trips a single train can complete
        route_not_completed = self.data.nb_routes

        # verify whether first route starts at the initial depot
        if seq[0] != route_not_completed:
            if not self.data.is_valid_route(0, 0, seq[0]):
                return False
        else:
            return False

        flag = False  # flag to tell whether train completes any trips during the day
        last_idx = len(seq) - 1
        for i in range(last_idx):
            cur = seq[i]
            nxt = seq[i + 1]

            if cur != route_not_completed:
                flag = True
                # verify compatibility between consecutive routes
                if nxt != route_not_completed:
                    if self.data.are_incompatible_routes(cur, nxt):
                        return False
            else:
                # after a non-trip, no trips can exist
                if nxt != route_not_completed:
                    return False

        return flag

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


