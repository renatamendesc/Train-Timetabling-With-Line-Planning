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
instance = sys.argv[1]

file_name = "script-solution.txt"
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
departure_stations = []

arrival_times = []
arrival_stations = []

info = data.split("---\n")

num_points = int((info[0].split(" "))[1])
trains = info[1].split("\n\n")

for t, train in enumerate(trains):
    trips = train.split("\n")

    departure_times.append([])
    arrival_times.append([])
    for i, trip in enumerate(trips):
        arcs = trip.split(" // ")

        departure_times[t].append([])
        arrival_times[t].append([])

        for p in range(0, num_points):
            departure_times[t][i].append([])
            arrival_times[t][i].append([])

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
            departure_times[int(departure_indexes.split(",")[0])][int(departure_indexes.split(",")[1])][int(departure_indexes.split(",")[2])] = int(departure_value)

            arrival_indexes = arrival.split(": ")[0]
            arrival_value = arrival.split(": ")[1]
            arrival_times[int(arrival_indexes.split(",")[0])][int(arrival_indexes.split(",")[1])][int(arrival_indexes.split(",")[2])] = int(arrival_value)

            total_times.append(departure_times[int(departure_indexes.split(",")[0])][int(departure_indexes.split(",")[1])][int(departure_indexes.split(",")[2])])
            total_times.append(arrival_times[int(arrival_indexes.split(",")[0])][int(arrival_indexes.split(",")[1])][int(arrival_indexes.split(",")[2])])

            total_stations.append(int(departure_indexes.split(",")[2]))
            total_stations.append(int(arrival_indexes.split(",")[2]))

        plt.plot(total_times, total_stations, color=color_train[t]) 

plt.xlabel('Horário')
plt.ylabel('Estação')
plt.title('Gráfico GHT')

ax = plt.gca()
ax.xaxis.set_major_locator(MaxNLocator(integer=True))
ax.yaxis.set_major_locator(MaxNLocator(integer=True))

file_graph = "solutions/graphs/" + instance + ".png"
plt.savefig(file_graph, dpi=300, bbox_inches='tight')
plt.show()

plt.close()
os.remove(file_name)
