import sys
import psutil
import time

from data import Data
from model import ModelTrainTimetabling
from enumeration import Enumeration
from heuristic import Heuristic

# solver = "HiGHS"
# solver = "GUROBI"

def main():
    # read instance
    if len(sys.argv) < 2:
        print("Error: Instance not provided!")
        return 1
    instance_path = sys.argv[1]
    data = Data(instance_path)
    data.read_data()
    data.print_data()

    # read method
    if len(sys.argv) < 3:
        print("Error: Method not provided!")
        return 1
    method = sys.argv[2]

    # read number of threads
    max_threads = psutil.cpu_count(logical=False)
    if len(sys.argv) < 4:
        print("Number of threads not provided! - Using default number of threads: 1")
        threads = 1
    else:
        threads = int(sys.argv[3])
        if threads < 1 or threads > max_threads:
            print("Error: Number of threads is not valid!")
            return 1

    # read solver ("HiGHS" or "GUROBI")
    solver = 0
    if len(sys.argv) < 5:
        print("Solver not provided! - Using default solver: Gurobi")
        solver = "GUROBI"
    else:
        solver = sys.argv[4]
        if solver != "HiGHS" and solver != "GUROBI":
            print("Error: Solver is not valid!")
            return 1

    print("\n\t================================================================")
    print(f"\tSolving instance {data.instance_name} with {method} and {threads} thread(s)...")
    print("\t================================================================")
    print(f"\t>> Instance: {data.instance_name}")
    print(f"\t>> Method: {method}")
    print(f"\t>> Number of threads: {threads}\n")

    # execute selected method
    if method == "model":
        data.change_scale()

        start_time = time.time()
        model = ModelTrainTimetabling(data, threads, 21600, 21600, solver)
        model.initialize()
        model.execute_solver_for_full_model()
        end_time = time.time()
        total_time = end_time - start_time
        print(f"\n-> Total time = {total_time:.2f}", end="")

        model.current_solution.rescale_values("model")

        model.current_solution.display_solution(data, "model")
        model.current_solution.save_solution(data, total_time, "model", solver, False, threads)
        model.current_solution.create_graph(data, "model", solver, threads)
        
    elif method == "enum":
        enumeration = Enumeration(data, threads, 21600, 3600, solver)
        enumeration.execute_enumeration()

    elif method == "heuristic":
        # heuristic = Heuristic(data, threads, 14400, 3600, solver)
        heuristic = Heuristic(data, threads, 21600, 3600, solver)
        heuristic.execute_heuristic()
    else:
        print("Did not provide a valid method!")
        return 1

    return 0

if __name__ == "__main__":
    main()