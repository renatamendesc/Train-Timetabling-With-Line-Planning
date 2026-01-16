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
                inst["feature_num_trains"] = int(lines[i+1])
                i += 2

            # elif key == "num_trips":
            #     inst["num_trips"] = list(map(int, lines[i+1].split()))
            #     i += 2

            elif key == "num_intervals":
                inst["feature_num_intervals"] = int(lines[i+1])
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
                inst["feature_num_points"] = int(lines[i+1])
                i += 2

            elif key == "num_stations":
                inst["feature_num_stations"] = int(lines[i+1])
                i += 2

            # elif key == "stations":
            #     inst["stations"] = list(map(int, lines[i+1].split()))
            #     i += 2

            elif key == "num_crossings":
                inst["feature_num_crossings"] = int(lines[i+1])
                i += 2

            # elif key == "crossings":
            #     inst["crossings"] = list(map(int, lines[i+1].split()))
            #     i += 2

            elif key == "num_depots":
                inst["feature_num_depots"] = int(lines[i+1])
                i += 2

            # elif key == "depots":
            #     inst["depots"] = list(map(int, lines[i+1].split()))
            #     i += 2

            # (don´t think this is a relevant info)
            # elif key == "initial_point":
            #     inst["initial_point"] = int(lines[i+1])
            #     i += 2

            elif key == "num_routes":
                inst["feature_num_routes"] = int(lines[i+1])
                i += 2

            # elif key == "routes":
            #     routes = []
            #     j = i + 1
            #     for _ in range(inst["num_routes"]):
            #         routes.append(list(map(int, lines[j].split())))
            #         j += 1
            #     inst["routes"] = routes
            #     i = j

            # elif key == "service_time_min":
            #     inst["service_time_min"] = list(map(float, lines[i+1].split()))
            #     i += 2

            # elif key == "service_time_max":
            #     inst["service_time_max"] = list(map(float, lines[i+1].split()))
            #     i += 2

            # elif key == "cost_matrix":
            #     # read header
            #     header = lines[i+1].split()
            #     n = len(header)
            #     mat = []
            #     j = i + 2
            #     for _ in range(n):
            #         row = list(map(float, lines[j].split()[1:]))
            #         mat.append(row)
            #         j += 1
            #     inst["cost_matrix"] = np.array(mat)
            #     i = j

            # elif key == "demands":
            #     demands = []
            #     j = i + 1
            #     while j < len(lines) and not lines[j].startswith("#"):
            #         demands.append(float(lines[j]))
            #         j += 1
            #     inst["demands"] = np.array(demands)
            #     i = j

            elif key == "max_time":
                inst["feature_max_time"] = float(lines[i+1])
                i += 2

            elif key == "alpha":
                inst["feature_alpha"] = float(lines[i+1])
                i += 2

            else:
                # Ignore unknown blocks - skip until next block starting with #
                i += 1
                while i < len(lines) and not lines[i].startswith("#"):
                    i += 1

        else:
            i += 1

    return inst

def main(instance_set, out_csv="metadata.csv"):
    rows = []
    
    # convert instance_set to Path and get all .txt files in the directory
    instance_dir = Path(instance_set)
    instance_files = sorted(instance_dir.glob("*.txt"))

    for fpath in instance_files:
        inst = read_instance(fpath)
        # pending: feats = extract_features(inst)
        # pending: extract algo times
        inst["Instances"] = Path(fpath).stem
        rows.append(inst)

    df = pd.DataFrame(rows)
    df = df.set_index("Instances")
    df.to_csv(out_csv)

    print(f"Metadata file written to {out_csv}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 create-metadata.py <instance_directory>")
        sys.exit(1)

    main(sys.argv[1]) # giving the instance directory as argument