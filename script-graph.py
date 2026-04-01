# python script to create graphs that represents the solution given by the model
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator
import re
import os
import sys

# define values
START_TIME = 18000
color_train = ['#FF0000', '#00FF00', '#0000FF', '#FFFF00', '#FF00FF', '#00FFFF', '#800080', '#FFA500', '#008000', '#FFC0CB', '#FFD700', '#000000', '#FFFFFF', '#808080', '#FF4500']

# get name of the instance
instance_set, type_execution, instance = sys.argv[1].split("/")

file_name = "../script-solution.txt"
# cleaning possible extra lines in the file
with open(file_name, "r") as file:
    content = file.read()
cleaned_content = re.sub(r"\n{3,}", "\n\n", content)
with open(file_name, "w") as file:
    file.write(cleaned_content)

# opening the file to get the solution 
with open(file_name, "r") as file:
   data = file.read()

departure_times = []
arrival_times = []

info = data.split("---\n")

num_points = int((info[0].split(" "))[1])
trains = info[1].split("\n\n")

triple_re = re.compile(r"(\d+),(\d+),(\d+):\s*([0-9]+(?:\.[0-9]+)?)")

# First pass: discover max (train, trip) indices used in the file.
max_train_idx = -1
max_trip_idx_by_train = {}
for m in triple_re.finditer(info[1]):
    t = int(m.group(1))
    i = int(m.group(2))
    max_train_idx = max(max_train_idx, t)
    max_trip_idx_by_train[t] = max(max_trip_idx_by_train.get(t, -1), i)

if max_train_idx < 0:
    raise ValueError("No (train,trip,point) entries found in script-solution.txt")

# Allocate arrays sized by discovered indices.
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

file_graph = "benchmarking/" + instance_set + "/" + type_execution + "/" + instance + "/graph.png"
print("Graph saved to", file_graph)
# file_graph = "graph-test.png"
plt.savefig(file_graph, dpi=300, bbox_inches='tight')
# plt.show()

plt.close()
os.remove(file_name)
