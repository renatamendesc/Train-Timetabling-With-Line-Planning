import copy

class Data:
    def __init__(self, instance_path):
        # instance name and set attributes
        self.instance_path = instance_path
        self.instance_name = None
        self.instance_set = None

        # number of trains
        self.nb_trains = None
        # maximum number of trips per train
        self.max_trips_per_train = None
        # maximum number of trips a train can complete
        self.max_nb_trips = None

        # number of points
        self.nb_points = None
        self.initial_point = None # initial point for all trains
        # types of points
        self.is_station = None
        self.is_crossing = None
        self.is_depot = None

        # data referring to the network of the train line (vertices and arcs)
        self.upper_vertices = None
        self.lower_vertices = None
        self.point_to_vertices = None
        self.vertex_to_point = None
        self.arcs = None
        self.vertex_out_arcs = None
        self.vertex_inc_arcs = None

        # set of possible collisions (incompatible points)
        self.inc_points = None

        # number of routes
        self.nb_routes = None
        # routes arcs and vertices
        self.route_arcs = None
        self.route_vertices = None

        # time intervals
        self.time_intervals = None
        # demands of each vertex by time intervals
        self.demands = None
        # sum of all demands that must be followd on a day for each vertex
        self.demand_per_day = None

        # distance values
        self.distance = None
        # distance and min service values
        self.distance_and_service_min = None
        # distance and max service values
        self.distance_and_service_max = None

        # maximum time for all of the trips to end
        self.max_time = None
        # alpha - headway time
        self.alpha = None

    # getters
    def get_nb_vertices(self):
        return self.nb_points * 2
    def get_nb_intervals(self):
        return len(self.time_intervals)

    # =====================================================================
    #            Util methods to get relevant information
    # =====================================================================
    def is_reversal_arc(self, arc):
        return abs(arc["out"] - arc["inc"]) == self.nb_points
    
    def can_be_adjacent_in_consecutive_trips(self, v1, v2):
        nb_routes_ending_at_v1 = 0
        nb_routes_starting_at_v2 = 0
        for route_idx in range(self.nb_routes):
            if self.finish_at_vertex(route_idx, v1):
                nb_routes_ending_at_v1 += 1
            if self.start_at_vertex(route_idx, v2):
                nb_routes_starting_at_v2 += 1
        return nb_routes_ending_at_v1 > 0 and nb_routes_starting_at_v2 > 0 and self.vertex_to_point[v1] == self.vertex_to_point[v2]
    
    def start_at_vertex(self, route_idx, vertex):
        return self.route_vertices[route_idx][0] == vertex
    def finish_at_vertex(self, route_idx, vertex):
        return self.route_vertices[route_idx][-1] == vertex
    
    def are_incompatible_routes(self, route_idx1, route_idx2):
        route1_last_vertex = self.route_vertices[route_idx1][-1]
        route2_first_vertex = self.route_vertices[route_idx2][0]
        return self.vertex_to_point[route1_last_vertex] != self.vertex_to_point[route2_first_vertex]
    
    def is_cyclic_route(self, route_idx):
        return self.vertex_to_point[self.route_vertices[route_idx][0]] == self.vertex_to_point[self.route_vertices[route_idx][-1]]
    def is_valid_route(self, train_idx, trip_idx, route_idx):
        if trip_idx == 0:
            return self.vertex_to_point[self.route_arcs[route_idx][0]["out"]] == self.initial_point
        return True
    def arc_belongs_to_route(self, route_idx, arc_idx):
        for arc in self.route_arcs[route_idx]:
            if arc["idx"] == arc_idx:
                return True
        return False
    
    def get_vertex_out_arcs(self, vertex):
        return self.vertex_out_arcs[vertex]
    def get_vertex_inc_arcs(self, vertex):
        return self.vertex_inc_arcs[vertex]
    
    # =====================================================================
    #            Methods to store data from arcs and inc points
    # =====================================================================
    def assign_arcs(self):
        self.arcs = []
        self.route_arcs = []

        # create arcs of the network
        for i in range(self.nb_points):

            # upper arcs
            if i < self.nb_points - 1:
                self.arcs.append({
                    "out": i,
                    "inc": i + 1,
                    "idx": len(self.arcs)
                })

            # lower arcs
            if i > 0:
                self.arcs.append({
                    "out": i + self.nb_points,
                    "inc": i + self.nb_points - 1,
                    "idx": len(self.arcs)
                })

            # reversal arcs
            if self.is_depot[i]:
                self.arcs.append({
                    "out": i,
                    "inc": i + self.nb_points,
                    "idx": len(self.arcs)
                })
                self.arcs.append({
                    "out": i + self.nb_points,
                    "inc": i,
                    "idx": len(self.arcs)
                })

        # create lists of lists for outcoming and incoming arcs
        nb_vertices = self.get_nb_vertices()
        self.vertex_out_arcs = [[] for _ in range(nb_vertices)]
        self.vertex_inc_arcs = [[] for _ in range(nb_vertices)]
        for v in range(nb_vertices):
            for arc in self.arcs:
                if arc["out"] == v:
                    self.vertex_out_arcs[v].append(arc)
                if arc["inc"] == v:
                    self.vertex_inc_arcs[v].append(arc)

        # for each route, we build the arcs
        for vertices in self.route_vertices:
            current_route_arcs = []
            for j in range(len(vertices) - 1):
                out_v = vertices[j]
                inc_v = vertices[j + 1]

                # find arc that belongs to the route
                for arc in self.arcs:
                    if arc["out"] == out_v and arc["inc"] == inc_v:
                        current_route_arcs.append(arc)
                        break
            self.route_arcs.append(current_route_arcs)

    def assign_incompatible_points(self):
        self.inc_points = []

        # for each upper vertex that is a crossing
        for i in range(self.nb_points - 1):
            if self.is_crossing[self.vertex_to_point[i]]:
                
                # find the next crossing in the lower section
                next_crossing = -1
                for j in range(i + self.nb_points + 1, 2 * self.nb_points):
                    if self.is_crossing[self.vertex_to_point[j]]:
                        next_crossing = j
                        break

                assert next_crossing != -1

                # considering that the train in the upper section departs first
                k = i
                v = next_crossing
                q = next_crossing - self.nb_points - 1
                a = None
                for arc in self.arcs:
                    # find arc a = (q, q + 1)
                    if arc["out"] == q and arc["inc"] == q + 1:
                        a = arc
                        break

                self.inc_points.append((k, q, v, a))

                # considering that the train in the lower section departs first
                k = next_crossing
                v = i
                q = i + self.nb_points + 1
                a = None
                for arc in self.arcs:
                    # find arc a = (q, q - 1)
                    if arc["out"] == q and arc["inc"] == q - 1:
                        a = arc
                        break

                self.inc_points.append((k, q, v, a))
    
    # =====================================================================
    #            Methods to read and display data from the instance
    # =====================================================================
    def read_data(self):

        # extracting the name of the instance (without path and .txt)
        separate = "/"
        self.instance_name = self.instance_path.split(separate)[-1]
        self.instance_name = self.instance_name.split(".")[0]

        # extracting the set of the instance (folder name)
        separate = "/"
        self.instance_set = self.instance_path.split(separate)[-2]

        file = open(self.instance_path, 'r')

        def skip_hash_line():
            while True:
                line = file.readline()
                if not line:
                    return
                if line.strip().startswith('#'):
                    break
            pos = file.tell()
            next_line = file.readline()
            if next_line and not next_line.strip():
                return
            if next_line:
                file.seek(pos)

        # --- nb_trains ---
        print("   > Reading number of trains...")
        skip_hash_line()
        self.nb_trains = int(file.readline().strip())

        # --- max_trips_per_train ---
        print("   > Reading maximum number of trips per train...")
        skip_hash_line()
        line = file.readline()
        self.max_trips_per_train = list(map(int, line.split()))
        self.max_nb_trips = max(self.max_trips_per_train)

        # --- time_intervals ---
        print("   > Reading time intervals...")
        skip_hash_line()
        line = file.readline()
        nb_intervals = int(line.strip())
        skip_hash_line()
        self.time_intervals = []
        for _ in range(nb_intervals):
            line = file.readline()
            a, b = map(int, line.split())
            self.time_intervals.append((a, b))

        # --- nb_points ---
        print("   > Reading number of points...")
        skip_hash_line()
        self.nb_points = int(file.readline().strip())

        # assign upper_vertex_set and lower_vertex_set
        self.upper_vertices = []
        self.lower_vertices = []
        self.point_to_vertices = [[0, 0] for _ in range(self.nb_points)]
        self.vertex_to_point = []
        for i in range(self.nb_points):
            self.upper_vertices.append(i)
            self.vertex_to_point.append(i)
            self.point_to_vertices[i][0] = i
        for i in range(self.nb_points, 2 * self.nb_points):
            self.lower_vertices.append(i)
            self.vertex_to_point.append(i - self.nb_points)
            self.point_to_vertices[i - self.nb_points][1] = i

        # --- stations ---
        print("   > Reading stations...")
        skip_hash_line()
        skip_hash_line()
        line = file.readline()
        stations = list(map(int, line.split()))
        self.is_station = [False] * self.nb_points
        for station in stations:
            self.is_station[station] = True

        # --- crossings ---
        print("   > Reading crossings...")
        skip_hash_line()
        skip_hash_line()
        line = file.readline()
        crossings = list(map(int, line.split()))
        self.is_crossing = [False] * self.nb_points
        for crossing in crossings:
            self.is_crossing[crossing] = True

        # --- depots ---
        print("   > Reading depots...")
        skip_hash_line()
        skip_hash_line()
        line = file.readline()
        depots = list(map(int, line.split()))
        self.is_depot = [False] * self.nb_points
        for depot in depots:
            self.is_depot[depot] = True

        # --- initial point ---
        print("   > Reading initial point...")
        skip_hash_line()
        self.initial_point = int(file.readline().strip())

        # --- Rotas ---
        print("   > Reading routes...")
        skip_hash_line()
        self.nb_routes = int(file.readline().strip())
        skip_hash_line()
        nb_vertices = self.nb_points * 2
        self.route_vertices = []
        for _ in range(self.nb_routes):
            row = list(map(int, file.readline().split()))
            row = [v for v in row if v != -1]
            self.route_vertices.append(row)
        self.assign_arcs()

        # --- service time min ---
        print("   > Reading distance and service...")
        skip_hash_line()
        aux_min = list(map(int, file.readline().split()))
        # --- service time max ---
        skip_hash_line()
        aux_max = list(map(int, file.readline().split()))

        # --- cost matrix ---
        skip_hash_line() # "#cost_matrix"
        file.readline()  # skip header line with indices
        # read matrix
        distance_matrix = []
        for _ in range(nb_vertices):
            vals = list(map(int, file.readline().split()))
            distance_matrix.append(vals[1:])  # remove index of line
        
        # create map of (out, inc) -> arc index
        arc_index_map = {}
        for arc in self.arcs:
            arc_index_map[(arc["out"], arc["inc"])] = arc["idx"]
        
        # convert matrices to lists indexed by arc
        # initialize lists with -1 (invalid arc)
        self.distance = [-1] * len(self.arcs)
        self.distance_and_service_min = [-1] * len(self.arcs)
        self.distance_and_service_max = [-1] * len(self.arcs)
        # fill distances of valid arcs
        for i in range(nb_vertices):
            for j in range(nb_vertices):
                if (i, j) in arc_index_map:
                    arc_idx = arc_index_map[(i, j)]
                    dist = distance_matrix[i][j]
                    self.distance[arc_idx] = dist
                    
                    # distance and service min
                    if dist != -1 and dist != 0:
                        self.distance_and_service_min[arc_idx] = dist + aux_min[self.vertex_to_point[j]]
                    else:
                        self.distance_and_service_min[arc_idx] = dist
                    # distance and service max
                    if dist != -1:
                        self.distance_and_service_max[arc_idx] = dist + aux_max[self.vertex_to_point[j]]
                    else:
                        self.distance_and_service_max[arc_idx] = dist

        # --- demands ---
        print("   > Reading demands...")
        skip_hash_line()
        self.demands = []
        for _ in range(nb_vertices):
            vals = list(map(int, file.readline().split()))
            self.demands.append(vals)

        # calculate demand per day for each vertex
        self.demand_per_day = [0] * nb_vertices
        for i in range(nb_vertices):
            for j in range(self.get_nb_intervals()):
                self.demand_per_day[i] += self.demands[i][j]

        # --- max_time ---
        print("   > Reading maximum time...")
        skip_hash_line()
        self.max_time = int(file.readline().strip())

        # --- alpha ---
        print("   > Reading alpha...")
        skip_hash_line()
        self.alpha = int(file.readline().strip())

        self.assign_incompatible_points()

        file.close()

        print("   > Reading complete!")

    def print_data(self):
        print("\n\t======================================================================")
        print(f"\tPrinting instance {self.instance_name} from set {self.instance_set}...")
        print("\t======================================================================")
        
        print("number of trains: ", self.nb_trains)
        print("naximum number of trips per train: ", self.max_trips_per_train, end="\n\n")

        # display time intervals
        print(f"number of intervals = {self.get_nb_intervals()}")
        for i in range(self.get_nb_intervals()):
            # formatação semelhante ao setw(6) → largura fixa
            print(f"\t({self.time_intervals[i][0]}, {self.time_intervals[i][1]:6})", end="")
        print(end="\n\n")

        # display points, stations, crossings and depots
        print(f"number of points = {self.nb_points}")
        print(f"\tinitial point: {self.initial_point}")
        print("\tstations: ", end="")
        for i in range(self.nb_points):
            if self.is_station[i]:
                print(f"{i}", end=" ")
        print()
        print("\tcrossings: ", end="")
        for i in range(self.nb_points): 
            if self.is_crossing[i]:
                print(f"{i}", end=" ")
        print()
        print("\tdepots: ", end="")
        for i in range(self.nb_points):
            if self.is_depot[i]:
                print(f"{i}", end=" ")
        print(end="\n\n")

        # display sets of vertices
        print(f"number of vertices = {self.get_nb_vertices()}")
        print(f"\ttotal vertices: ", end="")
        for i in range(self.get_nb_vertices()):
            print(f"{i}", end=" ")
        print()
        print(f"\tupper vertices: ", end="")
        for v in self.upper_vertices:
            print(f"{v}", end=" ")
        print()
        print(f"\tlower vertices: ", end="")
        for v in self.lower_vertices:
            print(f"{v}", end=" ")
        print(end="\n\n")
        # vertices of points
        for i in range(self.nb_points):
            print(f"\tvertices of point #{i}: ", end="")
            print(f"({self.point_to_vertices[i][0]}, {self.point_to_vertices[i][1]})")
        print(end="\n")

        # display sets of arcs
        print(f"number of arcs = {len(self.arcs)}")
        print(f" - all arcs:")
        for i in range(len(self.arcs)):
            print(f"\tarc #{i}: ({self.arcs[i]['out']}, {self.arcs[i]['inc']})")
        print()
        print(f" - reversal arcs:")
        for i in range(len(self.arcs)):
            if self.is_reversal_arc(self.arcs[i]):
                print(f"\tarc #{i}: ({self.arcs[i]['out']}, {self.arcs[i]['inc']})")
        print(end="\n")

        # display sets of routes
        print(f"number of routes = {self.nb_routes}")
        print(f" - vertices of routes...")
        for i in range(self.nb_routes):
            print(f"\troute #{i}: ", end="")
            for j in range(len(self.route_vertices[i])):
                print(f"{self.route_vertices[i][j]}", end=" ")
            print()
        print(f" - arcs of routes...")
        for i in range(self.nb_routes):
            print(f"\troute #{i}: ", end="")
            for j in range(len(self.route_arcs[i])):
                print(f"({self.route_arcs[i][j]['out']}, {self.route_arcs[i][j]['inc']})", end=" ")
            print()
        print()

        # display incompatible routes
        print(f"incompatible routes: ", end="")
        for i in range(self.nb_routes):
            for j in range(self.nb_routes):
                if self.are_incompatible_routes(i, j):
                    print(f"({i}, {j})", end=" ")
        print(end="\n\n")

        # display vertices that can be adjacent in consecutive trips
        print(f"vertices that can be adjacent in consecutive trips...")
        for i in range(self.get_nb_vertices()):
            for j in range(self.get_nb_vertices()):
                if self.can_be_adjacent_in_consecutive_trips(i, j):
                    print(f"\t({i}, {j})")
        print(end="\n")

        # display set of incompatible points (collisions)
        print(f"incompatible points...")
        for i in range(len(self.inc_points)):
            print(f"\t[{self.inc_points[i][0]}, {self.inc_points[i][1]}, {self.inc_points[i][2]}, ({self.inc_points[i][3]['out']}, {self.inc_points[i][3]['inc']})]")
        print(end="\n")

        # display non cyclical routes
        print(f"cyclical routes: ", end="")
        for i in range(self.nb_routes):
            if self.is_cyclic_route(i):
                print(f"{i}", end=" ")
        print(end="\n")

        # display routes that trains can complete in each trip
        print(f"routes of trip...")
        for i in range(self.nb_trains):
            for j in range(self.max_trips_per_train[i]):
                print(f"\troutes that train #{i} can complete at trip #{j}: ", end="")
                for k in range(self.nb_routes):
                    if self.is_valid_route(i, j, k):
                        print(f"{k}", end=" ")
                print()
        print()

        # display distances by arc
        print(f"distances by arc...")
        for arc in self.arcs:
            arc_idx = arc["idx"]
            print(f"\tarc #{arc_idx} ({arc['out']}, {arc['inc']}): distance = {self.distance[arc_idx]}")
        print(end="\n")

        # display distances and service by arc
        print(f"distances and minimum service by arc...")
        for arc in self.arcs:
            arc_idx = arc["idx"]
            print(f"\tarc #{arc_idx} ({arc['out']}, {arc['inc']}): distance+service_min = {self.distance_and_service_min[arc_idx]}")
        print(end="\n")
        print(f"distances and maximum service by arc...")
        for arc in self.arcs:
            arc_idx = arc["idx"]
            print(f"\tarc #{arc_idx} ({arc['out']}, {arc['inc']}): distance+service_max = {self.distance_and_service_max[arc_idx]}")
        print(end="\n")

        # display demands of time intervals
        print(f"demands...")
        for i in range(self.get_nb_vertices()):
            print(f"\tdemands of vertex {i} at time interval: ", end="")
            for j in range(self.get_nb_intervals()):
                print(f"{self.demands[i][j]}", end=" ")
            print(end="\n")
        print(end="\n")

        # display maximum time
        print(f"maximum time = {self.max_time}")
        # display alpha
        print(f"alpha = {self.alpha}")

    



                

                