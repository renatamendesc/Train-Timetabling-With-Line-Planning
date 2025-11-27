import sys
import psutil
from data import Data

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
        print("Going to execute the model...")
        # model = ModelORTools()
        # model.initialize(data, 1)
        # model.create_full_model(data)
        # model.execute_solver_for_full_model(data)
        # model.get_value_of_variables(data)
        # print(f"\n-> Total time = {model.current_sol.computational_time:.4f}")
        # model.current_sol.display_solution(data)
        # model.current_sol.create_graph(data, method, 1)

    elif method == "enum":
        print("Going to execute the enumeration...")
        # Enumeration(data, threads, 43200, 3600)

    elif method == "heuristic":
        print("Going to execute the heuristic...")
        # Heuristic(data, threads, 43200, 1200)

    else:
        print("Did not provide a valid method!")
        return 1

    return 0


if __name__ == "__main__":
    main()