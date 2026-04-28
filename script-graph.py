# python script to create graphs that represents the solution given by the model
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator
import re
import os
import sys
from pathlib import Path

# define values
START_TIME = 18000
color_train = ['#FF0000', '#00FF00', '#0000FF', '#FFFF00', '#FF00FF', '#00FFFF', '#800080', '#FFA500', '#008000', '#FFC0CB', '#FFD700', '#000000', '#FFFFFF', '#808080', '#FF4500']

# get name of the instance
instance_set, type_execution, instance = sys.argv[1].split("/")

repo_root = Path(__file__).resolve().parent

# prefer repo-root script-solution.txt (independent of cwd); fallback to cwd.
solution_file = repo_root / "script-solution.txt"
if not solution_file.exists():
    candidate = Path("script-solution.txt")
    if candidate.exists():
        solution_file = candidate.resolve()
    else:
        raise FileNotFoundError("Could not find script-solution.txt")

# cleaning possible extra lines in the file
with open(solution_file, "r") as file:
    content = file.read()
cleaned_content = re.sub(r"\n{3,}", "\n\n", content)
with open(solution_file, "w") as file:
    file.write(cleaned_content)

# opening the file to get the solution 
with open(solution_file, "r") as file:
   data = file.read()

departure_times = []
arrival_times = []

info = data.split("---\n")

num_points = int((info[0].split(" "))[1])
trains = info[1].split("\n\n")

triple_re = re.compile(r"(\d+),(\d+),(\d+):\s*([0-9]+(?:\.[0-9]+)?)")

# discover max (train, trip) indices used in the file.
max_train_idx = -1
max_trip_idx_by_train = {}
for m in triple_re.finditer(info[1]):
    t = int(m.group(1))
    i = int(m.group(2))
    max_train_idx = max(max_train_idx, t)
    max_trip_idx_by_train[t] = max(max_trip_idx_by_train.get(t, -1), i)

if max_train_idx < 0:
    raise ValueError("No (train,trip,point) entries found in script-solution.txt")

# allocate arrays sized by discovered indices.
departure_times = [
    [
        [None for _ in range(num_points)]
        for _ in range(max_trip_idx_by_train.get(t, -1) + 1)
    ]
    for t in range(max_train_idx + 1)
]
arrival_times = [
    [
        [None for _ in range(num_points)]
        for _ in range(max_trip_idx_by_train.get(t, -1) + 1)
    ]
    for t in range(max_train_idx + 1)
]

for t, train in enumerate(trains):
    trips = train.split("\n")

    for i, trip in enumerate(trips):
        arcs = trip.split(" // ")
        arcs.pop()

        total_times = []
        total_stations = []
        
        for arc in arcs:
            vertices = arc.split(" -> ")

            departure = vertices[0]
            arrival = vertices[1]

            departure_indexes = departure.split(": ")[0]
            departure_value = departure.split(": ")[1]
            dt, di, dp = (int(x) for x in departure_indexes.split(","))
            departure_times[dt][di][dp] = int(round(float(departure_value)))

            arrival_indexes = arrival.split(": ")[0]
            arrival_value = arrival.split(": ")[1]
            at, ai, ap = (int(x) for x in arrival_indexes.split(","))
            arrival_times[at][ai][ap] = int(round(float(arrival_value)))

            total_times.append(departure_times[dt][di][dp])
            total_times.append(arrival_times[at][ai][ap])

            total_stations.append(dp)
            total_stations.append(ap)

        plt.plot(total_times, total_stations, color=color_train[t]) 

# plt.xlabel('Horário')
plt.xlabel('Time')
# plt.ylabel('Estação')
plt.ylabel('Stopping Point')

ax = plt.gca()
ax.xaxis.set_major_locator(MaxNLocator(integer=True))
ax.yaxis.set_major_locator(MaxNLocator(integer=True))

file_graph = repo_root / "solutions" / instance_set / type_execution / instance / "graph.png"
file_graph.parent.mkdir(parents=True, exist_ok=True)

print("Graph saved to", file_graph)
plt.savefig(str(file_graph), dpi=300, bbox_inches='tight')
plt.close()
solution_file.unlink(missing_ok=True)
