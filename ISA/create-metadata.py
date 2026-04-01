import numpy as np
import pandas as pd
# import networkx as nx
import sys
from pathlib import Path

def read_instance(filepath):
    with open(filepath, "r") as f:
        lines = [l.strip() for l in f if l.strip()]

    i = 0
    inst = {}
    while i < len(lines):
        line = lines[i]

        if line.startswith("#"):
            key = line[1:]

            if key == "num_trains":
                inst["num_trains"] = int(lines[i+1])
                i += 2

            elif key == "num_trips":
                inst["num_trips"] = list(map(int, lines[i+1].split()))
                i += 2

            elif key == "num_intervals":
                inst["num_intervals"] = int(lines[i+1])
                i += 2

            # elif key == "time_intervals":
            #     intervals = []
            #     j = i + 1
            #     for _ in range(inst["num_intervals"]):
            #         intervals.append(list(map(float, lines[j].split())))
            #         j += 1
            #     inst["time_intervals"] = np.array(intervals)
            #     i = j

            elif key == "num_points":
                inst["num_points"] = int(lines[i+1])
                i += 2

            elif key == "num_stations":
                inst["num_stations"] = int(lines[i+1])
                i += 2

            elif key == "num_crossings":
                inst["num_crossings"] = int(lines[i+1])
                i += 2

            elif key == "num_depots":
                inst["num_depots"] = int(lines[i+1])
                i += 2

            elif key == "num_routes":
                inst["num_routes"] = int(lines[i+1])
                i += 2

            # elif key == "routes":
            #     routes = []
            #     j = i + 1
            #     for _ in range(inst["num_routes"]):
            #         routes.append(list(map(int, lines[j].split())))
            #         j += 1
            #     inst["routes"] = routes
            #     i = j

            elif key == "service_time_min":
                inst["service_time_min"] = list(map(float, lines[i+1].split()))
                i += 2

            elif key == "service_time_max":
                inst["service_time_max"] = list(map(float, lines[i+1].split()))
                i += 2

            elif key == "cost_matrix":
                # read header
                header = lines[i+1].split()
                n = len(header)
                mat = []
                j = i + 2
                for _ in range(n):
                    row = list(map(float, lines[j].split()[1:]))
                    mat.append(row)
                    j += 1
                inst["cost_matrix"] = np.array(mat)
                i = j

            elif key == "demands":
                demands = []
                j = i + 1
                while j < len(lines) and not lines[j].startswith("#"):
                    row = list(map(float, lines[j].split()))
                    demands.append(row)
                    j += 1
                inst["demands"] = np.array(demands)
                i = j

            elif key == "max_time":
                inst["max_time"] = float(lines[i+1])
                i += 2

            elif key == "alpha":
                inst["alpha"] = float(lines[i+1])
                i += 2

            else:
                # ignore unknown blocks: skip until next block starting with #
                i += 1
                while i < len(lines) and not lines[i].startswith("#"):
                    i += 1

        else:
            i += 1

    return inst

def extract_features(inst):
    feats = {}

    # getting simple features
    feats["feature_num_trains"] = inst["num_trains"]
    feats["feature_num_intervals"] = inst["num_intervals"]
    feats["feature_num_points"] = inst["num_points"]
    feats["feature_num_stations"] = inst["num_stations"]
    feats["feature_num_crossings"] = inst["num_crossings"]
    feats["feature_num_depots"] = inst["num_depots"]
    feats["feature_num_routes"] = inst["num_routes"]
    feats["feature_max_time"] = inst["max_time"]
    feats["feature_alpha"] = inst["alpha"]

    # extracting features from vectores and matrices
    feats["feature_avg_num_trips"] = np.mean(inst["num_trips"])               # get average max number of trips per train
    feats["feature_avg_service_time_min"] = np.mean(inst["service_time_min"]) # get average of service time min
    feats["feature_avg_service_time_max"] = np.mean(inst["service_time_max"]) # get average of service time max
    
    # get average of arcs in cost matrix (excluding -1 and 0 values)
    feats["feature_avg_arcs_in_cost_matrix"] = np.mean(inst["cost_matrix"][(inst["cost_matrix"] != -1) & (inst["cost_matrix"] != 0)])
    
    # get average demands per day (calculate demands of a day)
    demands_per_day = [0]*(inst["num_points"]*2)
    for i in range(inst["num_points"]*2):
        for j in range(inst["num_intervals"]):
            demands_per_day[i] += inst["demands"][i][j]
    feats["feature_avg_demands_per_day"] = np.mean(demands_per_day)

    # new features (decide whether to keep them or not):
    # get ratio between points and trains
    feats["feature_ratio_points_trains"] = inst["num_points"] / inst["num_trains"]
    
    # get ratio between points and stations, crossings and depots
    feats["feature_ratio_points_stations"] = inst["num_points"] / inst["num_stations"]
    feats["feature_ratio_points_crossings"] = inst["num_points"] / inst["num_crossings"]
    feats["feature_ratio_points_depots"] = inst["num_points"] / inst["num_depots"]

    # get standard deviation of arcs in cost matrix
    feats["feature_std_arcs_in_cost_matrix"] = np.std(inst["cost_matrix"][(inst["cost_matrix"] != -1) & (inst["cost_matrix"] != 0)])

    return feats

def get_solution_value_from_benchmark(project_root, set_name, instance_name, method):
    # read solution value for one instance from a method's benchmark.txt
    p = project_root / "python" / "benchmarking" / set_name / method / "benchmark.txt"
    if not p.exists():
        return np.nan
    lines = p.read_text().splitlines()
    for i, line in enumerate(lines):
        if line.strip() == f"{instance_name}:":
            for j in range(i + 1, min(i + 5, len(lines))):
                if "Solution value" in lines[j] and "=" in lines[j]:
                    return float(lines[j].split("=")[1].strip())
            return np.nan
    return np.nan

def get_best_ub(project_root):
    # list of dicts: solution value per instance and per method (from each method's benchmark.txt)
    methods = ["model_1", "enum_1", "heuristic_1"]
    result = []
    for instance_set in ["5-to-9", "10-to-14", "15-to-19", "20-to-24", "real"]:
        instance_dir = project_root / "instances" / instance_set
        instance_files = sorted(instance_dir.glob("*.txt"))
        for fpath in instance_files:
            instance_name = fpath.name
            instance_stem = fpath.stem
            row = {"set": instance_set, "instance": instance_stem}
            for method in methods:
                row[method] = get_solution_value_from_benchmark(project_root, instance_set, instance_name, method)
            row["best_ub"] = np.nanmin([row[m] for m in methods])
            result.append(row)
    return result


def get_algo_gaps(filepath, method):
    # return % gap to best_ub for this instance and method
    set_name = Path(filepath).parent.name
    instance_name = Path(filepath).name
    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parent
    methods = ["model_1", "enum_1", "heuristic_1"]
    values = [get_solution_value_from_benchmark(project_root, set_name, instance_name, m) for m in methods]
    best_ub = np.nanmin(values)
    val = get_solution_value_from_benchmark(project_root, set_name, instance_name, method)
    if np.isnan(best_ub) or best_ub == 0 or np.isnan(val):
        return np.nan
    return ((val - best_ub) / best_ub) * 100

def get_algo_times(filepath, method):
    set_name = Path(filepath).parent.name
    instance_name = Path(filepath).name

    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parent
    p = project_root / "python" / "benchmarking" / set_name / method / "benchmark.txt"

    lines = p.read_text().splitlines()
    for i, line in enumerate(lines):
        if line.strip() == f"{instance_name}:":
            # search for total time in the next lines of the block
            for j in range(i + 1, min(i + 5, len(lines))):
                if "Total time" in lines[j] and "=" in lines[j]:
                    total_time = float(lines[j].split("=")[1].strip())
                    return total_time
            
            return np.nan # incomplete benchmark 
    raise ValueError(f"Instance {instance_name!r} not found in {p}")

def main(out_csv, metadata_type):
    rows = []
    project_root = Path(__file__).resolve().parent.parent

    for instance_set in ["5-to-9", "10-to-14", "15-to-19", "20-to-24", "real"]:
        instance_dir = project_root / "instances" / instance_set
        instance_files = sorted(instance_dir.glob("*.txt"))

        for fpath in instance_files:
            inst = read_instance(fpath)
            feats = extract_features(inst)

            feats["Instances"] = Path(fpath).stem

            if metadata_type == "gaps":
                feats["algo_Model"] = get_algo_gaps(fpath, "model_1")
                feats["algo_Enum"] = get_algo_gaps(fpath, "enum_1")
                feats["algo_Heuristic"] = get_algo_gaps(fpath, "heuristic_1")
            elif metadata_type == "times":
                feats["algo_Model"] = get_algo_times(fpath, "model_1")
                feats["algo_Enum"] = get_algo_times(fpath, "enum_1")
                feats["algo_Heuristic"] = get_algo_times(fpath, "heuristic_1")

            rows.append(feats)

    df = pd.DataFrame(rows)
    out_path = Path(out_csv)
    if not out_path.is_absolute():
        out_path = project_root / "ISA" / out_path
    df.to_csv(out_path, index=False)
    print(f"Metadata file written to {out_path}")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Error: Metadata type and output file not provided!")
        exit(1)

    out_csv = sys.argv[1]
    metadata_type = sys.argv[2]
    
    main(out_csv, metadata_type)