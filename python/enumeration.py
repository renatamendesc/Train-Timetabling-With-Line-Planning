from model import ModelTrainTimetabling
from combinations import Combinations
from solution import Solution

import threading
from concurrent.futures import ThreadPoolExecutor
import time
import copy
import math

class Enumeration:
    def __init__(self, data, nb_threads, time_limit_complete, time_limit_per_combination, solver):
        self.data = data

        self.nb_threads = nb_threads
        self.time_limit_complete = time_limit_complete
        self.time_limit_per_combination = time_limit_per_combination

        self.nb_trains = data.nb_trains

        self.comb = Combinations(data)
        self.total_nb_combinations = None

        self.overall_best_sol = Solution()

        self.solver = solver

        # thread local variables
        self.thread_local = threading.local()

        # locks needed
        self.progress_lock = threading.Lock()
        self.best_lock = threading.Lock()
        self.time_limit_lock = threading.Lock()
        
        # counter for solved combinations
        self.counter_solved = 0
        self.aux_progress = None
        
        # flag to tell if the time limit was reached
        self.time_limit_reached = False

    def solve_combination_task(self, count):

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
                        # Only copy when we have a better solution
                        # Use deepcopy as Solution contains nested structures
                        self.overall_best_sol = copy.deepcopy(model_thread.best_solution)

            # verify time limit before executing the solver
            if self.time_limit_reached or (time.time() - self.start_time > self.time_limit_complete):
                with self.time_limit_lock:
                    self.time_limit_reached = True
                with self.progress_lock:
                    self.counter_solved += 1
                return

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

        # Process tasks in batches to avoid memory issues
        # Batch size: 3x the number of threads to ensure pipeline stays full
        # This provides a buffer to prevent threads from being idle while
        # new tasks are being submitted
        batch_size = max(self.nb_threads * 3, 10)
        
        with ThreadPoolExecutor(max_workers=self.nb_threads) as executor:
            count = 0
            pending_futures = {}
            
            # Submit initial batch to fill the pipeline
            while count < self.total_nb_combinations and len(pending_futures) < batch_size:
                # verify time limit before submitting new task
                if self.time_limit_reached or (time.time() - self.start_time > self.time_limit_complete):
                    with self.time_limit_lock:
                        self.time_limit_reached = True
                    break
                
                future = executor.submit(self.solve_combination_task, count)
                pending_futures[future] = count
                count += 1
            
            # Process completed tasks and submit new ones as they finish
            # This loop ensures we immediately replace completed tasks with new ones
            while pending_futures:
                # Check for completed tasks (more efficient: check all at once)
                done = [f for f in pending_futures if f.done()]
                
                # Process completed tasks immediately
                for future in done:
                    try:
                        future.result()  # This will raise any exceptions that occurred
                    except Exception as e:
                        # Log error but continue processing
                        pass
                    del pending_futures[future]
                
                # Immediately submit new tasks to replace completed ones
                # This keeps the pipeline full and prevents thread idleness
                while count < self.total_nb_combinations and len(pending_futures) < batch_size:
                    # verify time limit before submitting new task
                    if self.time_limit_reached or (time.time() - self.start_time > self.time_limit_complete):
                        with self.time_limit_lock:
                            self.time_limit_reached = True
                        break
                    
                    future = executor.submit(self.solve_combination_task, count)
                    pending_futures[future] = count
                    count += 1
                
                # If time limit reached, try to cancel remaining pending tasks
                if self.time_limit_reached:
                    cancelled = 0
                    for future in list(pending_futures.keys()):
                        if future.cancel():
                            cancelled += 1
                            del pending_futures[future]
                    if cancelled > 0:
                        print(f"Cancelled {cancelled} pending tasks. Waiting for running tasks to finish...")
                    # Wait for remaining tasks to complete
                    for future in list(pending_futures.keys()):
                        try:
                            future.result()
                        except Exception:
                            pass
                    break
                
                # Only sleep if no tasks completed (to avoid busy waiting)
                # This ensures we check for new completions frequently
                if not done:
                    time.sleep(0.0001)  # Reduced sleep time for faster response

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
        
        if self.time_limit_reached:
            print("\n-> Time limit reached. Displaying best solution found so far...\n")
        else:
            print("\nFinished enumeration!\n")
        
        print(f"-> Total time = {total_time:.2f}", end="")
        self.overall_best_sol.display_solution(self.data)
        self.overall_best_sol.save_solution(self.data, total_time, "enum", self.nb_threads)
