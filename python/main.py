import sys
import psutil
from data import Data
from model import ModelTrainTimetabling
from enumeration import Enumeration

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

    print("\n\t================================================================")
    print(f"\tSolving instance {data.instance_name} with {method} and {threads} thread(s)...")
    print("\t================================================================")
    print(f"\t>> Instance: {data.instance_name}")
    print(f"\t>> Method: {method}")
    print(f"\t>> Number of threads: {threads}\n")

    # execute selected method
    if method == "model":
        model = ModelTrainTimetabling(data, threads, 43200)
        model.initialize()
        model.add_constraints()
        model.execute_solver_for_full_model()

    elif method == "enum":
        enumeration = Enumeration(data, threads, 43200, 1200)
        enumeration.execute_enumeration()

    elif method == "heuristic":
        print("Going to execute the heuristic...")
        # Heuristic(data, threads, 43200, 1200)

    else:
        print("Did not provide a valid method!")
        return 1

    return 0


if __name__ == "__main__":
    main()