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

            # elif key == "stations":
            #     inst["stations"] = list(map(int, lines[i+1].split()))
            #     i += 2

            elif key == "num_crossings":
                inst["num_crossings"] = int(lines[i+1])
                i += 2

            # elif key == "crossings":
            #     inst["crossings"] = list(map(int, lines[i+1].split()))
            #     i += 2

            elif key == "num_depots":
                inst["num_depots"] = int(lines[i+1])
                i += 2

            # elif key == "depots":
            #     inst["depots"] = list(map(int, lines[i+1].split()))
            #     i += 2

            # (don´t think this is a relevant info)
            # elif key == "initial_point":
            #     inst["initial_point"] = int(lines[i+1])
            #     i += 2

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
    # print(inst["demands"])
    demands_per_day = [0]*(inst["num_points"]*2)
    for i in range(inst["num_points"]*2):
        for j in range(inst["num_intervals"]):
            demands_per_day[i] += inst["demands"][i][j]
    # print(demands_per_day)
    feats["feature_avg_demands_per_day"] = np.mean(demands_per_day)

    # some features to be futurely added: 
    # - ratio between points and stations, crossings, depots; 
    # - size of a single interval;

    return feats

def get_algo_times(filepath, method):

    # print(method)

    set_name = Path(filepath).parent.name
    instance_name = Path(filepath).name

    # print(set_name) 
    # print(instance_name)  

    lines = open(f"../python/benchmarking/{set_name}/{method}/benchmark-gurobi-servidor.txt", "r").read().splitlines()
    for i, line in enumerate(lines):
        if line.strip() == f"{instance_name}:":
            total_time = float(lines[i+2].split("=")[1])
            break

    # print(total_time)
    return np.log10(total_time + 1) # log 10 to normalize the time

def main(instance_set, out_csv="metadata.csv"):
    rows = []
    
    # convert instance_set to Path and get all .txt files in the directory
    instance_dir = Path(instance_set)
    instance_files = sorted(instance_dir.glob("*.txt"))

    for fpath in instance_files:
        inst = read_instance(fpath)
        feats = extract_features(inst)
        feats["Instances"] = Path(fpath).stem
        # feats["algo_Model"] = get_algo_times(fpath, "model_1")
        # feats["algo_Enum"] = get_algo_times(fpath, "enum_1")
        feats["algo_Heuristic"] = get_algo_times(fpath, "heuristic_1")

        rows.append(feats)

    df = pd.DataFrame(rows)
    df = df.set_index("Instances")
    df.to_csv(out_csv)

    print(f"Metadata file written to {out_csv}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 create-metadata.py <instance_directory>")
        sys.exit(1)

    main(sys.argv[1]) # giving the instance directory as argument