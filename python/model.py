from mip import Model, xsum, BINARY, minimize, OptimizationStatus, GUROBI
from solution import Solution
import os
import copy

class ModelTrainTimetabling:

    # BIG_M = 100000

    def __init__(self, data, threads, time_limit, time_limit_per_combination, solver):
        self.data = data
        self.nb_threads = threads
        self.model = None

        self.time_limit = time_limit
        self.time_limit_per_combination = time_limit_per_combination

        # solution object
        self.best_solution = Solution()
        self.current_solution = Solution()
        
        # contador para gerar arquivos .lp únicos
        self.lp_file_counter = 0

        # decision variables
        self.z_ = None
        self.x_ = None
        self.x_bar_ = None
        self.y_ = None
        self.y_bar_ = None
        self.lambda_ = None
        self.w_ = None
        self.u_ = None

        # constraints sets
        self.routes_constraints = []

        self.solver = solver

    def initialize(self, find_feasible=False):
        self.model = Model(solver_name=self.solver)
        # create variables
        self.add_variables()
        # create objective function
        if not find_feasible:
            self.model.objective = minimize(self.z_)
        # add general constraints
        self.add_constraints()

    def reset(self, find_feasible=False):
        # recreates the model
        self.model = Model(solver_name=self.solver)
        self.add_variables()
        if not find_feasible:
            self.model.objective = minimize(self.z_)
        self.add_constraints()
        self.routes_constraints.clear()

    def create_model_for_combination(self, routes_of_trains):
        self.add_routes_constraints(routes_of_trains)

    def add_routes_constraints(self, routes_of_trains):
        self.routes_constraints = []
        # create routes constraints
        for t in range(len(routes_of_trains)):
            current = routes_of_trains[t]
            for i in range(len(current)):
                if i < self.data.max_trips_per_train[t]:

                    # if trip is not made
                    if current[i] == self.data.nb_routes:
                        # set all lambda to zero
                        for r in range(self.data.nb_routes):
                            if self.data.is_valid_route(t, i, r):
                                constr = self.model.add_constr(self.lambda_[t][i][r] == 0, name=f"route_lambda_zero({t})({i})({r})")
                                self.routes_constraints.append(constr)

                    # if a valid route exists for this trip
                    elif current[i] != -1:
                        if self.data.is_valid_route(t, i, current[i]):
                            # set route to 1
                            constr = self.model.add_constr(self.lambda_[t][i][current[i]] == 1, name=f"route_lambda_one({t})({i})({current[i]})")
                            self.routes_constraints.append(constr)

    def disable_trips(self, current_max_trips_per_train):
        for t in range(self.data.nb_trains):
            for i in range(self.data.max_trips_per_train[t]):
                if i >= current_max_trips_per_train[t]:
                    for r in range(self.data.nb_routes):
                        constr = self.model.add_constr(self.lambda_[t][i][r] == 0, name=f"route_lambda_zero({t})({i})({r})")
                        self.routes_constraints.append(constr)

    def add_variables(self):
        self.x_ = [
            [
                [
                    self.model.add_var(var_type=BINARY, name=f"x({t})({i})({a})")
                    for a in range(len(self.data.arcs))
                ]
                for i in range(self.data.max_trips_per_train[t])
            ]
            for t in range(self.data.nb_trains)
        ]

        self.x_bar_ = [
            [
                [
                    [
                        self.model.add_var(var_type=BINARY, name=f"x_bar({t})({i})({a})({h})")
                        for h in range(self.data.get_nb_intervals())
                    ]
                    for a in range(len(self.data.arcs))
                ]
                for i in range(self.data.max_trips_per_train[t])
            ]
            for t in range(self.data.nb_trains)
        ]

        self.y_ = [
            [
                [
                    self.model.add_var(lb=0, ub=float('inf'), name=f"y({t})({i})({v})")
                    for v in range(self.data.get_nb_vertices())
                ]
                for i in range(self.data.max_trips_per_train[t])
            ]
            for t in range(self.data.nb_trains)
        ]

        self.y_bar_ = [
            [
                [
                    self.model.add_var(lb=0, ub=float('inf'), name=f"y_bar({t})({i})({v})")
                    for v in range(self.data.get_nb_vertices())
                ]
                for i in range(self.data.max_trips_per_train[t])
            ]
            for t in range(self.data.nb_trains)
        ]

        self.lambda_ = [
            [
                [
                    self.model.add_var(var_type=BINARY, name=f"lambda({t})({i})({r})")
                    for r in range(self.data.nb_routes)
                ]
                for i in range(self.data.max_trips_per_train[t])
            ]
            for t in range(self.data.nb_trains)
        ]

        inc_set = {(inc_point[0], inc_point[2]) for inc_point in self.data.inc_points}
        self.w_ = [
            [
                [
                    [
                        [
                            [
                                self.model.add_var(var_type=BINARY, name=f"w({t})({i})({v})({l})({j})({k})") 
                                if t != l and (k, v) in inc_set else None
                                for k in range(self.data.get_nb_vertices())
                            ]
                            for j in range(self.data.max_trips_per_train[l])
                        ]
                        for l in range(self.data.nb_trains)
                    ]
                    for v in range(self.data.get_nb_vertices())
                ]
                for i in range(self.data.max_trips_per_train[t])
            ]
            for t in range(self.data.nb_trains)
        ]

        self.u_ = [
            [
                [
                    [
                        [
                            self.model.add_var(var_type=BINARY, name=f"u({t})({i})({l})({j})({v})")
                            if t != l else None
                            for v in range(self.data.get_nb_vertices())
                        ]
                        for j in range(self.data.max_trips_per_train[l])
                    ]
                    for l in range(self.data.nb_trains)
                ]
                for i in range(self.data.max_trips_per_train[t])
            ] 
            for t in range(self.data.nb_trains)
        ]

        self.z_ = self.model.add_var(lb=0, ub=float("inf"), name="z")

    def add_constraints(self):

        self.BIG_M = self.data.max_time * 2

        # constraints to get value of z (2)
        for t in range(self.data.nb_trains):
            for i in range(self.data.max_trips_per_train[t]):
                for l in range(self.data.nb_trains):
                    for j in range(self.data.max_trips_per_train[l]):
                        for p in range(self.data.nb_points):
                            if self.data.is_station[p]:

                                # upper vertex
                                v = self.data.point_to_vertices[p][0]
                                self.model += (
                                    self.z_ >= self.y_[t][i][v] - self.y_[l][j][v],
                                    f"max_gap_upper({t})({i})({l})({j})({v})"
                                )

                                # lower vertex
                                v = self.data.point_to_vertices[p][1]
                                self.model += (
                                    self.z_ >= self.y_[t][i][v] - self.y_[l][j][v],
                                    f"max_gap_lower({t})({i})({l})({j})({v})"
                                )

        # associate x variable with x_bar variable (3)
        for t in range(self.data.nb_trains):
            for i in range(self.data.max_trips_per_train[t]):
                for a in range(len(self.data.arcs)):
                    # sum of x_bar in all intervals
                    expr = xsum(self.x_bar_[t][i][a][h] for h in range(self.data.get_nb_intervals()))
                    # add constraint
                    self.model += (
                        self.x_[t][i][a] == expr,
                        f"associate_x_and_x_bar({t})({i})({a})"
                    )
        
        # constraints to associate routes with arcs (lambda variables with x variables) (4)
        for t in range(self.data.nb_trains):
            for i in range(self.data.max_trips_per_train[t]):
                for a in range(len(self.data.arcs)):
                    expr = xsum(
                        self.lambda_[t][i][r]
                        for r in range(self.data.nb_routes)
                        if self.data.is_valid_route(t, i, r) and self.data.arc_belongs_to_route(r, a)
                    )
                    self.model += (
                        self.x_[t][i][a] == expr,
                        f"associated_route_with_arcs({t})({i})({a})"
                    )

        # constraints to associate trips with routes (5)
        for t in range(self.data.nb_trains):
            for i in range(self.data.max_trips_per_train[t]):

                expr = xsum(
                    self.lambda_[t][i][r]
                    for r in range(self.data.nb_routes)
                    if self.data.is_valid_route(t, i, r)
                )

                self.model += (
                    expr <= 1,
                    f"associate_trip_with_route({t})({i})"
                )

        # establish incompatible routes (6)
        for t in range(self.data.nb_trains):
            for i in range(1, self.data.max_trips_per_train[t]):
                for r1 in range(self.data.nb_routes):
                    if self.data.is_valid_route(t, i, r1):
                        for r2 in range(self.data.nb_routes):
                            if self.data.is_valid_route(t, i - 1, r2):
                                if self.data.are_incompatible_routes(r2, r1):

                                    self.model += (
                                        self.lambda_[t][i][r1] +
                                        self.lambda_[t][i - 1][r2] <= 1,
                                        f"incompatible_routes({t})({i - 1})({i})({r1})({r2})"
                                    )

        # constraints to establish subsequential trips according to indexes (7)
        for t in range(self.data.nb_trains):
            for i in range(1, self.data.max_trips_per_train[t]):

                expr1 = xsum(
                    self.lambda_[t][i][r]
                    for r in range(self.data.nb_routes)
                    if self.data.is_valid_route(t, i, r)
                )

                expr2 = xsum(
                    self.lambda_[t][i - 1][r]
                    for r in range(self.data.nb_routes)
                    if self.data.is_valid_route(t, i - 1, r)
                )

                self.model += (expr1 <= expr2,
                        f"subseq_trips({t})({i})({i - 1})")

        # constraints to connect trips of the same train - trip can only start after the previous one has ended (8)
        for t in range(self.data.nb_trains):
            for i in range(1, self.data.max_trips_per_train[t]):
                for v in range(self.data.get_nb_vertices()):
                    for k in range(self.data.get_nb_vertices()):
                        if self.data.can_be_adjacent_in_consecutive_trips(v, k):

                            expr1 = xsum(
                                self.lambda_[t][i - 1][r]
                                for r in range(self.data.nb_routes)
                                if self.data.finish_at_vertex(r, v)
                                and self.data.is_valid_route(t, i - 1, r)
                            )

                            expr2 = xsum(
                                self.lambda_[t][i][r]
                                for r in range(self.data.nb_routes)
                                if self.data.start_at_vertex(r, k)
                                and self.data.is_valid_route(t, i, r)
                            )

                            self.model += (
                                self.y_[t][i][k] >= self.y_bar_[t][i - 1][v] - self.BIG_M * (2 - expr1 - expr2),
                                f"connect_trips({t})({i})({v})({k})"
                            )

        # constraints to establish arrival times, considering the traveling times (9) and (10)
        for t in range(self.data.nb_trains):
            for i in range(self.data.max_trips_per_train[t]):
                for r in range(self.data.nb_routes):
                    if self.data.is_valid_route(t, i, r):
                        for arc in self.data.route_arcs[r]:
                            v = arc["out"]
                            k = arc["inc"]
                            a = arc["idx"]

                            self.model += (
                                self.y_bar_[t][i][k] >= self.y_[t][i][v] + self.data.distance[a] - self.BIG_M * (1 - self.lambda_[t][i][r]),
                                f"traveling_time1({t})({i})({r})"
                            )
                            self.model += (
                                self.y_bar_[t][i][k] <= self.y_[t][i][v] + self.data.distance[a] + self.BIG_M * (1 - self.lambda_[t][i][r]),
                                f"traveling_time2({t})({i})({r})"
                            )

        # constraints to establish minimum and maximum service times (11) and (12)
        for t in range(self.data.nb_trains):
            for i in range(self.data.max_trips_per_train[t]):
                for r in range(self.data.nb_routes):
                    if self.data.is_valid_route(t, i, r):
                        route_arcs = self.data.route_arcs[r]
                        for arc_pos in range(len(route_arcs) - 1): 
                            arc = route_arcs[arc_pos]
                            v = arc["out"]
                            k = arc["inc"]
                            a = arc["idx"]

                            # minimum service time
                            self.model += (
                                self.y_[t][i][k] >= self.y_[t][i][v] + self.data.distance_and_service_min[a] - self.BIG_M * (1 - self.lambda_[t][i][r]),
                                f"service_time_min({t})({i})({r})"
                            )
                            # maximum service time
                            self.model += (
                                self.y_[t][i][k] <= self.y_[t][i][v] + self.data.distance_and_service_max[a] + self.BIG_M * (1 - self.lambda_[t][i][r]),
                                f"service_time_max({t})({i})({r})"
                            )

        # constraints to establish the maximum time for the simulation, and sets y = 0 and y_bar = 0 when vertices are not visited (13) and (14)
        for t in range(self.data.nb_trains):
            for i in range(self.data.max_trips_per_train[t]):
                for v in range(self.data.get_nb_vertices()):

                    # outcoming vertices
                    expr1 = xsum(self.x_[t][i][arc["idx"]] for arc in self.data.get_vertex_out_arcs(v))

                    # incoming vertices
                    expr2 = xsum(self.x_[t][i][arc["idx"]] for arc in self.data.get_vertex_inc_arcs(v))

                    # maximum time for departure
                    self.model += (
                        self.y_[t][i][v] <= self.data.max_time * expr1,
                        f"max_time_out({t})({i})({v})"
                    )
                    # maximum time for arrival
                    self.model += (
                        self.y_bar_[t][i][v] <= self.data.max_time * expr2,
                        f"max_time_inc({t})({i})({v})"
                    )

        # constraints to establish time intervals (15) and (16)
        for t in range(self.data.nb_trains):
            for i in range(self.data.max_trips_per_train[t]):
                for v in range(self.data.get_nb_vertices()):
                    for h in range(self.data.get_nb_intervals()):
                        h_start = self.data.time_intervals[h][0]
                        h_final = self.data.time_intervals[h][1]

                        for arc in self.data.get_vertex_out_arcs(v):
                            a = arc["idx"]

                            # start of interval
                            self.model += (
                                self.y_[t][i][v] >= h_start - self.BIG_M * (1 - self.x_bar_[t][i][a][h]),
                                f"intervals_start({t})({i})({v})({h})"
                            )
                            # end of interval
                            self.model += (
                                self.y_[t][i][v] <= h_final + self.BIG_M * (1 - self.x_bar_[t][i][a][h]),
                                f"intervals_final({t})({i})({v})({h})"
                            )

        # demands by each time interval (17)
        for v in range(self.data.get_nb_vertices()): 
            for h in range(self.data.get_nb_intervals()):
                expr = xsum(
                    self.x_bar_[t][i][arc["idx"]][h]
                    for t in range(self.data.nb_trains)
                    for i in range(self.data.max_trips_per_train[t])
                    for arc in self.data.get_vertex_out_arcs(v)
                )
                self.model += (
                    expr >= self.data.demands[v][h],
                    f"demands({v})({h})"
                )

        # associate w variable with x variable (18), (19) and (20)
        for t in range(self.data.nb_trains) :
            for l in range(self.data.nb_trains):
                if t != l:
                    for i in range(self.data.max_trips_per_train[t]):
                        for j in range(self.data.max_trips_per_train[l]):

                            for inc_point in self.data.inc_points:
                                k = inc_point[0]
                                v = inc_point[2]

                                if v == k:
                                    continue
                                
                                expr1 = xsum(
                                    self.x_[t][i][arc["idx"]]
                                    for arc in self.data.get_vertex_out_arcs(v)
                                    if not self.data.is_reversal_arc(arc)
                                )

                                expr2 = xsum(
                                    self.x_[l][j][arc["idx"]]
                                    for arc in self.data.get_vertex_out_arcs(k)
                                    if not self.data.is_reversal_arc(arc)
                                )

                                self.model += (
                                    self.w_[t][i][v][l][j][k] <= expr1,
                                    f"link_w_and_x1({t})({i})({v})({l})({j})({k})"
                                )
                                self.model += (
                                    self.w_[t][i][v][l][j][k] <= expr2,
                                    f"link_w_and_x2({t})({i})({v})({l})({j})({k})"
                                )
                                
                                # Check if w_[l][j][k][t][i][v] also exists
                                self.model += (
                                    self.w_[t][i][v][l][j][k] + self.w_[l][j][k][t][i][v] >= expr1 + expr2 - 1,
                                    f"link_w_and_x3({t})({i})({v})({l})({j})({k})"
                                )

        # associate u variable with x variable (21), (22) and (23)
        for t in range(self.data.nb_trains):
            for l in range(self.data.nb_trains):
                if t != l:
                    for i in range(self.data.max_trips_per_train[t]):
                        for j in range(self.data.max_trips_per_train[l]):
                            for v in range(self.data.get_nb_vertices()):

                                expr1 = xsum(
                                    self.x_[t][i][arc["idx"]]
                                    for arc in self.data.get_vertex_out_arcs(v)
                                )

                                expr2 = xsum(
                                    self.x_[l][j][arc["idx"]]
                                    for arc in self.data.get_vertex_out_arcs(v)
                                )

                                self.model += (
                                    self.u_[t][i][l][j][v] <= expr1,
                                    f"link_u_and_x1({t})({i})({v})({l})({j})"
                                )
                                self.model += (
                                    self.u_[t][i][l][j][v] <= expr2,
                                    f"link_u_and_x2({t})({i})({v})({l})({j})"
                                )
                                self.model += (
                                    self.u_[t][i][l][j][v] + self.u_[l][j][t][i][v] >= expr1 + expr2 - 1,
                                    f"link_u_and_x3({t})({i})({v})({l})({j})"
                                )

        # constraints to avoid collisions in different directions (24)
        for t in range(self.data.nb_trains):
            for l in range(self.data.nb_trains):
                if t != l:
                    for i in range(self.data.max_trips_per_train[t]):
                        for j in range(self.data.max_trips_per_train[l]):
                            for inc_point in self.data.inc_points:
                                k = inc_point[0]
                                q = inc_point[1]
                                v = inc_point[2]

                                # k = inc_point[0]
                                # q = inc_point[1]
                                # v = inc_point[2]
                                # a = inc_point[3]["idx"]

                                self.model += (
                                    self.y_[t][i][v] >= self.y_bar_[l][j][q] - self.BIG_M * (1 - self.w_[t][i][v][l][j][k]),
                                    f"collisions_diff_directions({t})({i})({v})({l})({j})({k})"
                                )

                                # self.model += (
                                #     self.y_[t][i][v] >= self.y_[l][j][q] + self.data.distance[a] - self.BIG_M * (1 - self.w_[t][i][v][l][j][k]),
                                #     f"collisions_diff_directions({t})({i})({v})({l})({j})({k})"
                                # )

        # constraints to avoid collisions in the same direction (overtakings) (25)
        for t in range(self.data.nb_trains):
            for l in range(self.data.nb_trains):
                if t != l:
                    for i in range(self.data.max_trips_per_train[t]):
                        for j in range(self.data.max_trips_per_train[l]):
                            for v in range(self.data.get_nb_vertices()):
                                self.model += (
                                    self.y_bar_[t][i][v] >= self.y_[l][j][v] - self.BIG_M * (1 - self.u_[t][i][l][j][v]),
                                    f"collisions_same_direction({t})({i})({l})({j})({v})"
                                )

        # minimum headway constraints (26)
        for t in range(self.data.nb_trains):
            for l in range(self.data.nb_trains):
                if t != l:
                    for i in range(self.data.max_trips_per_train[t]):
                        for j in range(self.data.max_trips_per_train[l]):
                            for v in range(self.data.get_nb_vertices()):
                                self.model += (
                                    self.y_[t][i][v] >= self.y_[l][j][v] + self.data.alpha - self.BIG_M * (1 - self.u_[t][i][l][j][v]),
                                    f"headway({t})({i})({l})({j})({v})"
                                )

    def store_value_of_variables(self):
        self.current_solution.x_values = [
            [
                [self.x_[t][i][a].x for a in range(len(self.x_[t][i]))]
                for i in range(len(self.x_[t]))
            ]
            for t in range(len(self.x_))
        ]
        self.current_solution.x_bar_values = [
            [
                [
                    [self.x_bar_[t][i][a][h].x for h in range(len(self.x_bar_[t][i][a]))]
                    for a in range(len(self.x_bar_[t][i]))
                ]
                for i in range(len(self.x_bar_[t]))
            ]
            for t in range(len(self.x_bar_))
        ]
        self.current_solution.y_values = [
            [
                [self.y_[t][i][v].x for v in range(len(self.y_[t][i]))]
                for i in range(len(self.y_[t]))
            ]
            for t in range(len(self.y_))
        ]
        self.current_solution.y_bar_values = [
            [   
                [self.y_bar_[t][i][v].x for v in range(len(self.y_bar_[t][i]))]
                for i in range(len(self.y_bar_[t]))
            ]
            for t in range(len(self.y_bar_))
        ]
        self.current_solution.lambda_values = [
            [
                [self.lambda_[t][i][r].x for r in range(len(self.lambda_[t][i]))]
                for i in range(len(self.lambda_[t]))
            ]
            for t in range(len(self.lambda_))
        ]
        self.current_solution.w_values = [
            [
                [
                    [
                        [
                            [
                                self.w_[t][i][v][l][j][k].x if self.w_[t][i][v][l][j][k] is not None else None
                                for k in range(len(self.w_[t][i][v][l][j]))
                            ]
                            for j in range(len(self.w_[t][i][v][l]))
                        ]
                        for l in range(len(self.w_[t][i][v]))
                    ]
                    for v in range(len(self.w_[t][i]))
                ]
                for i in range(len(self.w_[t]))
            ]
            for t in range(len(self.w_))
        ]
        self.current_solution.u_values = [
            [
                [
                    [
                        [
                            self.u_[t][i][l][j][v].x if self.u_[t][i][l][j][v] is not None else None
                            for v in range(len(self.u_[t][i][l][j]))
                        ]
                        for j in range(len(self.u_[t][i][l]))
                    ]
                    for l in range(len(self.u_[t][i]))
                ]
                for i in range(len(self.u_[t]))
            ]
            for t in range(len(self.u_))
        ]

    def execute_solver_for_full_model(self):
        # setting parameters
        self.model.threads = self.nb_threads

        status = self.model.optimize(max_seconds=self.time_limit)
        if status in [OptimizationStatus.INFEASIBLE, OptimizationStatus.NO_SOLUTION_FOUND]:
            print("Could not find a feasible solution!")
            return False

        # a feasible solution was found
        self.current_solution.obj_value = self.model.objective_value
        self.get_gap_value()
        self.store_value_of_variables()
        self.current_solution.feasible = True

        if status != OptimizationStatus.OPTIMAL:
            self.current_solution.proven_optimal = False
        return True

    def execute_solver_for_combination(self, method, best_bound):
        # setting parameters
        self.model.threads = 1
        self.model.cutoff = best_bound
        self.model.verbose = 0
        self.model.mip_gap = 0.01 # 1% tolerance for the objective value

        status = self.model.optimize(max_seconds=self.time_limit_per_combination)

        if status in [OptimizationStatus.INFEASIBLE, OptimizationStatus.NO_SOLUTION_FOUND]:
            return False

        # a feasible solution was found
        if method == "enum":
            if self.model.objective_value < self.best_solution.obj_value:
                self.current_solution.obj_value = self.model.objective_value
                self.store_value_of_variables()
                self.current_solution.feasible = True

                if status != OptimizationStatus.OPTIMAL:
                    # if any solution for combination is not proven optimal, 
                    # we cannot prove global optimality of the best solution
                    self.best_solution.proven_optimal = False

                self.best_solution = copy.deepcopy(self.current_solution)
            return True
        
        if method == "heuristic":
            if self.model.objective_value < self.best_solution.obj_value:
                self.current_solution.obj_value = self.model.objective_value
                self.store_value_of_variables()
                self.current_solution.extract_routes_combination(self.data)
                self.current_solution.feasible = True

                self.best_solution = copy.deepcopy(self.current_solution)
            elif self.model.objective_value == self.best_solution.obj_value:
                self.store_value_of_variables()
                self.current_solution.extract_routes_combination(self.data)

                if self.current_solution.max_nb_repeated_routes > self.best_solution.max_nb_repeated_routes:
                    self.current_solution.obj_value = self.model.objective_value
                    self.current_solution.feasible = True

                    if status == OptimizationStatus.OPTIMAL:
                        self.current_solution.proven_optimal = True

                    self.best_solution = copy.deepcopy(self.current_solution)
            return True

    def get_gap_value(self):
        # calculate gap value
        self.current_solution.lower_bound = self.model.objective_bound # lower bound
        if self.current_solution.obj_value != 0:
            self.current_solution.gap_value = abs(self.current_solution.lower_bound - self.current_solution.obj_value) / abs(self.current_solution.obj_value)
        else:
            self.current_solution.gap_value = None # not a valid gap value