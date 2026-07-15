# python script to create graphs that represent the solution
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator
from pathlib import Path
import sys

# colors
COLOR_TRAIN = [
    "#FF0000", "#00AA00", "#0000FF", "#FF9900",
    "#AA00AA", "#00AAAA", "#880000", "#008800",
    "#000088", "#666600", "#880088", "#008888",
    "#444444", "#AA5500", "#5555AA"
]

# ---------------------------------------------------------------------

instance_set, execution_type, instance = sys.argv[1].split("/")

repo_root = Path(__file__).resolve().parent

solution_file = repo_root / "script-solution.txt"

if not solution_file.exists():
    raise FileNotFoundError(solution_file)

# ---------------------------------------------------------------------

with open(solution_file) as f:
    lines = f.readlines()

num_points = int(lines[0].split()[1])

fig, ax = plt.subplots(figsize=(16, 8))

# ---------------------------------------------------------------------

for line in lines[2:]:

    line = line.strip()

    if line == "":
        continue

    # remove o último //
    if line.endswith("//"):
        line = line[:-2].strip()

    arcs = [a.strip() for a in line.split("//") if a.strip()]

    if len(arcs) == 0:
        continue

    times = []
    stations = []

    train = None

    for arc in arcs:

        departure, arrival = arc.split("->")

        departure = departure.strip()
        arrival = arrival.strip()

        dep_info, dep_time = departure.split(":")
        arr_info, arr_time = arrival.split(":")

        dep_time = float(dep_time.strip())
        arr_time = float(arr_time.strip())

        t, trip, dep_station = map(int, dep_info.strip().split(","))
        _, _, arr_station = map(int, arr_info.strip().split(","))

        train = t

        times.extend([dep_time, arr_time])
        stations.extend([dep_station, arr_station])

    ax.plot(
        times,
        stations,
        color=COLOR_TRAIN[train % len(COLOR_TRAIN)],
        linewidth=2,
        marker="o",
        markersize=3
    )

# ---------------------------------------------------------------------

ax.set_xlabel("Time")
ax.set_ylabel("Stopping Point")

ax.set_xlim(left=0)
ax.set_ylim(-0.5, num_points - 0.5)

ax.xaxis.set_major_locator(MaxNLocator(integer=True))
ax.yaxis.set_major_locator(MaxNLocator(integer=True))

ax.grid(True, linestyle="--", alpha=0.4)

plt.tight_layout()

# ---------------------------------------------------------------------

output = (
    repo_root
    / "solutions"
    / instance_set
    / execution_type
    / instance
    / "graph.png"
)

output.parent.mkdir(parents=True, exist_ok=True)

plt.savefig(output, dpi=300)
plt.close()

solution_file.unlink(missing_ok=True)

print(f"Graph saved to {output}")