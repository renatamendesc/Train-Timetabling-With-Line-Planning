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

    def extract_routes_combination(self, data):
        self.routes_combination = []
        for t in range(self.data.nb_trains):
            for i in range(self.data.max_trips_per_train[t]):
                for r in range(self.data.nb_routes):
                    if self.lambda_values[t][i][r] > 0:
                        routes_combination.append(r)

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

    def create_graph(self):
        pass
