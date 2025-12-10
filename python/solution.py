import os

class Solution:
    def __init__(self):
        self.obj_value = float('inf')
        self.gap_value = 0

        self.proven_optimal = False
        self.feasible = False

        # value of variables
        self.z_values = None
        self.x_values = None
        self.x_bar_values = None
        self.y_values = None
        self.y_bar_values = None
        self.lambda_values = None
        self.w_values = None
        self.u_values = None

        # routes combination
        self.routes_combination = None
        self.max_nb_repeated_routes = None

    def extract_routes_combination(self, data):
        self.routes_combination = []
        for t in range(data.nb_trains):
            train_routes = []
            for i in range(data.max_trips_per_train[t]):
                for r in range(data.nb_routes):
                    if self.lambda_values[t][i][r] > 0:
                        train_routes.append(r)
                        break  # only one route per trip
            self.routes_combination.append(train_routes)

        self.store_max_nb_repeated_route(data)

    def store_max_nb_repeated_route(self, data):
        times_route_is_completed = [0] * data.nb_routes

        for train_routes in self.routes_combination:
            for route in train_routes:
                if route != data.nb_routes:
                    times_route_is_completed[route] += 1

        self.max_nb_repeated_routes = max(times_route_is_completed)

    def display_value_of_variables(self):
        pass

    def display_solution(self, data):
        print("\n-> Solution value =", round(self.obj_value))
        print("-> Gap value =", self.gap_value)
        print()

        def convert_time(seconds):
            hours = seconds // 3600
            minutes = (seconds % 3600) // 60
            seconds = seconds % 60
            return f"{int(hours):02d}:{int(minutes):02d}:{int(seconds):02d}"

        for t in range(data.nb_trains):
            print("=============")
            print("Train", t)
            print("=============")
            
            for i in range(data.max_trips_per_train[t]):
                for r in range(data.nb_routes):
                    if data.is_valid_route(t, i, r):
                        if self.lambda_values[t][i][r] > 0:
                            print("> Trip", i)
                            
                            for arc in data.route_arcs[r]:
                                departure = arc["out"]
                                arrival = arc["inc"]
                                
                                print(
                                    f"   {departure}(time {int(self.y_values[t][i][departure])} - {convert_time(self.y_values[t][i][departure])})"
                                    f"(time {int(self.y_bar_values[t][i][arrival])} - {convert_time(self.y_bar_values[t][i][arrival])}) -> ",
                                    end=""
                                )
                            print(arrival)

    def save_solution(self, data, total_time, method, threads):

        output_file = f"benchmarking/{data.instance_set}/{method}_{threads}/{data.instance_name}/timetable.txt"
        
        # Criar diretórios se não existirem
        os.makedirs(os.path.dirname(output_file), exist_ok=True)

        with open(output_file, 'w') as f:
            f.write(f"-> Total time = {total_time:.2f}\n")
            f.write(f"-> Solution value = {round(self.obj_value)}\n")
            f.write(f"-> Gap value = {self.gap_value}\n")
            f.write("\n")
            
            def convert_time(seconds):
                hours = seconds // 3600
                minutes = (seconds % 3600) // 60
                seconds = seconds % 60
                return f"{int(hours):02d}:{int(minutes):02d}:{int(seconds):02d}"
            
            for t in range(data.nb_trains):
                f.write("=============\n")
                f.write(f"Train {t}\n")
                f.write("=============\n")
                
                for i in range(data.max_trips_per_train[t]):
                    for r in range(data.nb_routes):
                        if data.is_valid_route(t, i, r):
                            if self.lambda_values[t][i][r] > 0:
                                f.write(f"> Trip {i}\n")
                                
                                for arc in data.route_arcs[r]:
                                    departure = arc["out"]
                                    arrival = arc["inc"]
                                    
                                    f.write(
                                        f"   {departure}(time {int(self.y_values[t][i][departure])} - {convert_time(self.y_values[t][i][departure])})"
                                        f"(time {int(self.y_bar_values[t][i][arrival])} - {convert_time(self.y_bar_values[t][i][arrival])}) -> "
                                    )
                                f.write(f"{arrival}\n")
        
        print(f"\nSolution saved to {output_file}")

    def create_graph(self):
        pass
