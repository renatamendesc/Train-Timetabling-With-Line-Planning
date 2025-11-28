from model import ModelTrainTimetabling
from combinations import Combinations
from solution import Solution

import threading
from concurrent.futures import ThreadPoolExecutor, as_completed
import time
import copy
import math

class Enumeration:
    def __init__(self, data, nb_threads, time_limit_complete, time_limit_per_combination):
        self.data = data

        self.nb_threads = nb_threads
        self.time_limit_complete = time_limit_complete
        self.time_limit_per_combination = time_limit_per_combination

        self.comb = Combinations(data)
        self.total_nb_combinations = None

        self.overall_best_sol = Solution()

        self.start_time = None
        self.end_time = None

        # create models for each thread
        self.models = [ModelTrainTimetabling(self.data, self.nb_threads, self.time_limit_complete, self.time_limit_per_combination) for _ in range(self.nb_threads)]
        for i in range(self.nb_threads):
            self.models[i].initialize()

        # locks needed
        self.progress_lock = threading.Lock()
        self.best_lock = threading.Lock()

    def solve_combination_task(self, count):

        thread_id = threading.get_ident()

        print(f"Thread {thread_id} is working on combination {count}")

        # get current combination
        idx = count
        indices = [0]*self.nb_trains
        for k in reversed(range(self.nb_trains)):
            size = len(self.comb.all_trips_combinations[k])
            indices[k] = idx % size
            idx //= size
        current_combination = [
            self.comb.all_trips_combinations[k][indices[k]]
            for k in range(self.nb_trains)
        ]

        print(current_combination)

        # check feasibility

        # create model for the thread
        model_thread = self.models[thread_id]
        model_thread.reset()
        model_thread.create_model_for_combination(current_combination)
        feasible = model_thread.execute_solver_for_combination("enum")

        if feasible:
            with self.best_lock:
                if model_thread.best_solution.obj_value < self.overall_best_sol.obj_value:
                    self.overall_best_sol = copy.deepcopy(model_thread.best_solution)

        with self.progress_lock:
            self.counter_solved += 1
            if self.counter_solved % aux_progress == 0:
                print(f"{(self.counter_solved//aux_progress)*10}% done - "
                      f"{self.counter_solved}/{self.total_nb_combinations} combination(s) tested! "
                      f"(Thread {thread_id})")

    def execute_all_combinations(self):
            
        aux_progress = math.ceil(0.1 * self.total_nb_combinations)
        if aux_progress == 0:
            aux_progress = 1

        with ThreadPoolExecutor(max_workers=self.nb_threads) as executor:
            for count in range(self.total_nb_combinations):
                executor.submit(self.solve_combination_task, count)

        executor.shutdown(wait=True)

    def execute_enumeration(self):
        # calculate total number of possible combinations
        self.start_time = time.time()

        self.comb.calculate_trips_combinations()
        self.total_nb_combinations = 1
        for i in range(len(self.comb.all_trips_combinations)):
            self.total_nb_combinations *= len(self.comb.all_trips_combinations[i])

        print(f"Starting to test combinations... - Total number of combinations: {self.total_nb_combinations}")
        self.execute_all_combinations()

        self.end_time = time.time()

        total_time = self.end_time - self.start_time
        print("\nFinished enumeration.")
        print(f"Best solution found: {self.overall_best_sol.obj_value}")
        print(f"Total time: {total_time:.2f} sec")