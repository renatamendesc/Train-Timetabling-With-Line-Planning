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

        self.nb_trains = data.nb_trains

        self.comb = Combinations(data)
        self.total_nb_combinations = None

        self.overall_best_sol = Solution()

        # thread local variables
        self.thread_local = threading.local()

        # locks needed
        self.progress_lock = threading.Lock()
        self.best_lock = threading.Lock()
        
        # counter for solved combinations
        self.counter_solved = 0
        self.aux_progress = None

    def solve_combination_task(self, count):

        # # verify time limit
        # if time.time() - self.start_time > self.time_limit_complete:
        #     print("Reached time limit!")
        #     return

        if not hasattr(self.thread_local, "model"):
            self.thread_local.model = ModelTrainTimetabling(self.data, self.nb_threads, self.time_limit_complete, self.time_limit_per_combination)
            self.thread_local.model.initialize()

        model_thread = self.thread_local.model

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

        # check feasibility
        if self.comb.is_valid_combination(current_combination):
            # if valid, reset model for the thread
            model_thread.reset()
            model_thread.create_model_for_combination(current_combination)
            feasible = model_thread.execute_solver_for_combination("enum", self.overall_best_sol.obj_value)

            if feasible:
                with self.best_lock:
                    if model_thread.best_solution.obj_value < self.overall_best_sol.obj_value:
                        self.overall_best_sol = copy.deepcopy(model_thread.best_solution)

        with self.progress_lock:
            self.counter_solved += 1
            if self.aux_progress and self.counter_solved % self.aux_progress == 0:
                percentage = (self.counter_solved / self.total_nb_combinations) * 100
                print(f"{percentage:.1f}% done - "
                      f"{self.counter_solved}/{self.total_nb_combinations} combination(s) tested! ")

    def execute_all_combinations(self):
            
        self.aux_progress = math.ceil(0.1 * self.total_nb_combinations)
        if self.aux_progress == 0:
            self.aux_progress = 1

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

        print(f"Starting to test combinations... - Total number of combinations: {self.total_nb_combinations}\n")
        self.execute_all_combinations()

        end_time = time.time()

        total_time = end_time - self.start_time
        print("\nFinished enumeration!\n")
        print(f"-> Total time = {total_time:.2f}", end="")
        self.overall_best_sol.display_solution(self.data)
        self.overall_best_sol.save_solution(self.data, total_time, "enum", self.nb_threads)
