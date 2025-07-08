#include "Model.hpp"

using namespace std;

double MyIncumbentCallback::best_solution_time = -1;

void Model::initialize (Data &data)
{
    env = IloEnv();
    model = IloModel(env);
    constraints = IloConstraintArray(env);
    obj = IloExpr(env);
}

void Model::reset (Data &data)
{
    env.end();
    initialize(data);
}

void Model::run (Data &data)
{
    // create decision variables
    add_variables(data);

    // create objective function
    obj = z_; 
    model.add(IloMinimize(env, obj));
    constraints.add(z_ >= 0);

    // create constraints
    add_constraints(data);
    model.add(constraints);

    // extract solution from the model
    extract_solution(data, true);
}

int Model::run_LP_with_routes_constraints (Data &data, vector<vector<int>> &routes_of_trains)
{
    // create decision variables
    add_variables(data);

    // create objective function
    obj = z_; 
    model.add(IloMinimize(env, obj));
    constraints.add(z_ >= 0);

    // create constraints
    add_constraints(data);

    // create routes constraints
    for (int t = 0; t < routes_of_trains.size(); t++)
    {
        vector<int> current = routes_of_trains[t];
        for (int i = 0; i < current.size(); i++)
        {
            if (i < data.get_train_max_trips(t))
            {       
                // if trip is not made                           
                if (current[i] == data.get_nb_routes())
                {
                    // assign variable equal to zero
                    for (int r = 0; r < data.get_nb_routes(); r++)
                    {
                        constraints.add(lambda_[t][i][r] == 0);
                    }
                }
                else if (current[i] != -1)
                {
                    // variable is 1 if route is completed
                    constraints.add(lambda_[t][i][current[i]] == 1); 
                }
            }
        }
    }
    model.add(constraints);

    // extract solution from the model
    return extract_solution(data, false);
}

int Model::run_MIP_with_routes_constraints (Data &data, vector<vector<int>> &routes_of_trains)
{
    // create decision variables
    add_variables(data);

    // create objective function
    obj = z_; 
    model.add(IloMinimize(env, obj));
    constraints.add(z_ >= 0);

    // create constraints
    add_constraints(data);

    for (int t = 0; t < routes_of_trains.size(); t++)
    {
        vector<int> current = routes_of_trains[t];
        for (int i = 0; i < current.size(); i++)
        {
            if (i < data.get_train_max_trips(t))
            {       
                // if trip is not made                           
                if (current[i] == data.get_nb_routes())
                {
                    // assign variable equal to zero
                    for (int r = 0; r < data.get_nb_routes(); r++)
                    {
                        constraints.add(lambda_[t][i][r] == 0);
                    }
                }
                else if (current[i] != -1)
                {
                    constraints.add(lambda_[t][i][current[i]] == 1);
                }
            }
        }
    }
    model.add(constraints);

    // extract solution from the model
    return extract_solution(data, false);
}

void Model::add_variables (Data &data)
{
    // create variable x - specifies whether train t on trip i uses arc a
    x_ = NumVarMatrix3d(env, data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        x_[t] = NumVarMatrix2d(env, data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            x_[t][i] = IloNumVarArray(env, data.get_nb_arcs());
            for (int a = 0; a < data.get_nb_arcs(); a++)
            {
                x_[t][i][a] = IloNumVar(env, 0, 1, ILOBOOL);
                x_[t][i][a].setName(string("x(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(a) + ")").c_str());
            }
        }
    }

    // create variable x bar - specifies whether train t on trip i uses arc a on time interval h
    x_bar_ = NumVarMatrix4d(env, data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        x_bar_[t] = NumVarMatrix3d(env, data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            x_bar_[t][i] = NumVarMatrix2d(env, data.get_nb_arcs());
            for (int a = 0; a < data.get_nb_arcs(); a++)
            {
                x_bar_[t][i][a] = IloNumVarArray(env, data.get_nb_arcs());
                for (int h = 0; h < data.get_nb_intervals(); h++)
                {
                    x_bar_[t][i][a][h] = IloNumVar(env, 0, 1, ILOBOOL);
                    x_bar_[t][i][a][h].setName(string("x_bar(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(a) + ")(" + to_string(h) + ")").c_str());
                }
            }
        }
    }

    // create variable y - departure time at a vertex
    y_ = NumVarMatrix3d(env, data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        y_[t] = NumVarMatrix2d(env, data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            y_[t][i] = IloNumVarArray(env, data.get_nb_vertices());
            for (int v = 0; v < data.get_nb_vertices(); v++)
            {
                y_[t][i][v] = IloNumVar(env, 0, IloInfinity);
                y_[t][i][v].setName(string("y(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")").c_str());
            }
        }
    }

    // create variable y bar - arrival time at a vertex
    y_bar = NumVarMatrix3d(env, data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        y_bar[t] = NumVarMatrix2d(env, data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            y_bar[t][i] = IloNumVarArray(env, data.get_nb_vertices());
            for (int v = 0; v < data.get_nb_vertices(); v++)
            {
                y_bar[t][i][v] = IloNumVar(env, 0, IloInfinity);
                y_bar[t][i][v].setName(string("y_bar(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")").c_str());
            }
        }
    }

    // create variable lambda - specifies whether train t on trip i uses route r
    lambda_ = NumVarMatrix3d(env, data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        lambda_[t] = NumVarMatrix2d(env, data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            lambda_[t][i] = IloNumVarArray(env, data.get_nb_routes());
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                if (data.is_valid_route(t, i, r))
                {
                    lambda_[t][i][r] = IloNumVar(env, 0, 1, ILOBOOL);
                    lambda_[t][i][r].setName(string("lambda(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(r) + ")").c_str());
                }
            }
        }
    }
    
    // create variable w - specifies whether train t departs from vertex v after train l departs from vertex k
    w_ = NumVarMatrix6d(env, data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        w_[t] = NumVarMatrix5d(env, data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            w_[t][i] = NumVarMatrix4d(env, data.get_nb_vertices());
            for (int v = 0; v < data.get_nb_vertices(); v++)
            {
                w_[t][i][v] = NumVarMatrix3d(env, data.get_nb_trains());
                for (int l = 0; l < data.get_nb_trains(); l++)
                {
                    w_[t][i][v][l] = NumVarMatrix2d(env, data.get_train_max_trips(l));
                    for (int j = 0; j < data.get_train_max_trips(l); j++)
                    {
                        w_[t][i][v][l][j] = IloNumVarArray(env, data.get_nb_vertices());
                        for (int k = 0; k < data.get_nb_vertices(); k++)
                        {
                            w_[t][i][v][l][j][k] = IloNumVar(env, 0, 1, ILOBOOL);
                            w_[t][i][v][l][j][k].setName(string("w(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(k) + ")").c_str());
                        }
                    }
                }
            }
        }
    }

    // create variable u - specifies whether train t departs from vertex v after train l
    u_ = NumVarMatrix5d(env, data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        u_[t] = NumVarMatrix4d(env, data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            u_[t][i] = NumVarMatrix3d(env, data.get_nb_trains());
            for (int l = 0; l < data.get_nb_trains(); l++)
            {
                u_[t][i][l] = NumVarMatrix2d(env, data.get_train_max_trips(l));
                for (int j = 0; j < data.get_train_max_trips(l); j++)
                {
                    u_[t][i][l][j] = IloNumVarArray(env, data.get_nb_vertices());
                    for (int v = 0; v < data.get_nb_vertices(); v++)
                    {
                        u_[t][i][l][j][v] = IloNumVar(env, 0, 1, ILOBOOL);
                        u_[t][i][l][j][v].setName(string("u(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(v) + ")").c_str());
                    }
                }
            }
        }
    }

    // create variable z - maximum gap between depatures of subsequential trips of the same train from the same station and direction (maximum headway)
    z_ = IloNumVar(env, -IloInfinity, IloInfinity);
    z_.setName(string("z").c_str());  
}

void Model::add_constraints (Data &data) 
{
    // constraints to get value of z (max headway) (2)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            for (int l = 0; l < data.get_nb_trains(); l++)
            {
                for (int j = 0; j < data.get_train_max_trips(l); j++)
                {
                    for (int p = 0; p < data.get_nb_points(); p++)
                    {
                        if (data.is_station(p))
                        {
                            int v;
                            v = data.get_point_vertices(p)[0];
                            constraints.add(z_ >= y_[t][i][v] - y_[l][j][v]);
                            constraints[constraints.getSize() - 1].setName(string("max_gap_between_departures_upper(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(v) + ")").c_str());

                            v = data.get_point_vertices(p)[1];
                            constraints.add(z_ >= y_[t][i][v] - y_[l][j][v]);
                            constraints[constraints.getSize() - 1].setName(string("max_gap_between_departures_lower(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(v) + ")").c_str());
                        }
                    }
                }
            }
        }
    }

    // associate x variable with x_bar variable (3)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            for (int a = 0; a < data.get_nb_arcs(); a++)
            {
                IloExpr expr(env);
                for (int h = 0; h < data.get_nb_intervals(); h++)
                {
                    expr += x_bar_[t][i][a][h];
                }
                constraints.add(x_[t][i][a] == expr);
                constraints[constraints.getSize() - 1].setName(string("associate_x_and_x_bar(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(a) + ")").c_str());
            }
        }
    }

    // constraints to associate routes with arcs (lambda values with x values) (4)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            for (int a = 0; a < data.get_nb_arcs(); a++)
            {
                IloExpr expr(env);
                for (int r = 0; r < data.get_nb_routes(); r++)
                {
                    if (data.is_valid_route(t, i, r) && data.arc_belongs_to_route(r, a))
                    {
                        expr += lambda_[t][i][r];
                    }
                }
                constraints.add(x_[t][i][a] == expr);
                constraints[constraints.getSize() - 1].setName(string("associated_route_with_arcs(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(a) + ")").c_str());
            }
        }
    }

    // each trip completed must be associated with at most one route (5)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            IloExpr expr(env);
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                if (data.is_valid_route(t, i, r))
                {
                    expr += lambda_[t][i][r];
                }
            }

            constraints.add(expr <= 1);
            constraints[constraints.getSize() - 1].setName(string("associate_trip_with_route(" + to_string(t) + ")(" + to_string(i) + ")").c_str());
        }
    }

    // establish incompatible routes (6)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 1; i < data.get_train_max_trips(t); i++)
        {
            for (int r1 = 0; r1 < data.get_nb_routes(); r1++)
            {
                if (data.is_valid_route(t, i, r1))
                {
                    for (int r2 = 0; r2 < data.get_nb_routes(); r2++)
                    {
                        if (data.is_valid_route(t, i - 1, r2))
                        {
                            if (data.are_incompatible_routes(r2, r1))
                            {
                                constraints.add(lambda_[t][i][r1] + lambda_[t][i - 1][r2] <= 1);
                                constraints[constraints.getSize() - 1].setName(string("incompatible_routes(" + to_string(t) + ")(" + to_string(i - 1) + ")(" + to_string(i) + ")(" + to_string(r1) + ")(" + to_string(r2) + ")").c_str());
                            }
                        }
                    }
                }
            }
        }
    }

    // constraints to establish subsequential trips according to indexes (7)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 1; i < data.get_train_max_trips(t); i++)
        {

            IloExpr expr1(env);
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                if (data.is_valid_route(t, i, r))
                {
                    expr1 += lambda_[t][i][r];
                }
            }
            IloExpr expr2(env);
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                if (data.is_valid_route(t, i - 1, r))
                {
                    expr2 += lambda_[t][i - 1][r];
                }
            }

            constraints.add(expr1 <= expr2);
            constraints[constraints.getSize() - 1].setName(string("subseq_trips(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(i - 1) + ")").c_str());
        }
    }

    // constraints to connect trips of the same train - trip can only start after the previous one has ended (8)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 1; i < data.get_train_max_trips(t); i++)
        {
            for (int v = 0; v < data.get_nb_vertices(); v++)
            {
                for (int k = 0; k < data.get_nb_vertices(); k++)
                {
                    if (data.can_be_adjacent_in_consecutive_trips(v, k))
                    {
                        IloExpr expr1(env);
                        for (int r = 0; r < data.get_nb_routes(); r++)
                        {
                            if (data.finish_at_vertex(r, v) && data.is_valid_route(t, i - 1, r))
                            {
                                expr1 += lambda_[t][i - 1][r];
                            }
                        }
                        IloExpr expr2(env);
                        for (int r = 0; r < data.get_nb_routes(); r++)
                        {
                            if (data.start_at_vertex(r, k) && data.is_valid_route(t, i, r))
                            {
                                expr2 += lambda_[t][i][r];
                            }
                        }

                        constraints.add(y_[t][i][k] >= y_bar[t][i - 1][v] - BIG_M * (2 - expr1 - expr2));
                        constraints[constraints.getSize() - 1].setName(string("connect_trips(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(k) + ")").c_str());
                    }
                }
            }
        }
    }

    // constraints to establish arrival times, considering the traveling times (9) and (10)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                if (data.is_valid_route(t, i, r))
                {
                    for (auto arc : data.get_route_arcs(r))
                    {
                        int v = arc.out;
                        int k = arc.inc;
                        int a = arc.idx;

                        // arrival time is equal to the departure time plus the traveling time
                        constraints.add(y_bar[t][i][k] >= y_[t][i][v] + data.get_distance(a) - BIG_M * (1 - lambda_[t][i][r]));
                        constraints[constraints.getSize() - 1].setName(string("traveling_time1(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(r) + ")").c_str());
                        constraints.add(y_bar[t][i][k] <= y_[t][i][v] + data.get_distance(a) + BIG_M * (1 - lambda_[t][i][r]));
                        constraints[constraints.getSize() - 1].setName(string("traveling_time2(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(r) + ")").c_str());
                    }
                }
            }
        }
    }

    // constraints to establhish minimum and maximum service times (11) and (12)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                if (data.is_valid_route(t, i, r))
                {
                    for (int arc_pos = 0; arc_pos < data.get_route_arcs(r).size() - 1; arc_pos++) // not considering the last arc of the route
                    {
                        Arc arc = data.get_route_arcs(r)[arc_pos];

                        int v = arc.out;
                        int k = arc.inc;
                        int a = arc.idx;

                        // minimum service time
                        constraints.add(y_[t][i][k] >= y_[t][i][v] + data.get_distance_and_service_min(a) - BIG_M * (1 - lambda_[t][i][r]));
                        constraints[constraints.getSize() - 1].setName(string("service_time_min(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(r) + ")").c_str());
                    
                        // maximum service time
                        constraints.add(y_[t][i][k] <= y_[t][i][v] + data.get_distance_and_service_max(a) + BIG_M * (1 - lambda_[t][i][r]));
                        constraints[constraints.getSize() - 1].setName(string("service_time_max(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(r) + ")").c_str());
                    }
                }
            }
        }
    }

    // establishes the maximum time for the simulation and makes y = 0 and y_bar = 0 when vertices are not visited (13) and (14)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            for (int v = 0; v < data.get_nb_vertices(); v++)
            {

                // outcoming vertices
                IloExpr expr1(env);
                for (auto arc : data.get_vertex_out_arcs(v))
                {
                    int a = arc.idx;
                    expr1 += x_[t][i][a];
                }

                // incoming vertices
                IloExpr expr2(env);
                for (auto arc : data.get_vertex_inc_arcs(v))
                {
                    int a = arc.idx;
                    expr2 += x_[t][i][a];
                }

                constraints.add(y_[t][i][v] <= data.get_max_time() * expr1);
                constraints[constraints.getSize() - 1].setName(string("max_time_out(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")").c_str());
                
                constraints.add(y_bar[t][i][v] <= data.get_max_time() * expr2);
                constraints[constraints.getSize() - 1].setName(string("max_time_inc(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")").c_str());
            }
        }
    }

    // constraints to establish time intervals (15) and (16)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            for (int v = 0; v < data.get_nb_vertices(); v++)
            {
                for (int h = 0; h < data.get_nb_intervals(); h++)
                {
                    int h_start = data.get_interval_start(h);
                    int h_final = data.get_interval_end(h);

                    for (auto arc : data.get_vertex_out_arcs(v))
                    {
                        int a = arc.idx;
                        constraints.add(y_[t][i][v] >= h_start - BIG_M * (1 - x_bar_[t][i][a][h]));
                        constraints[constraints.getSize() - 1].setName(string("intervals_start(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(h) + ")").c_str());

                        constraints.add(y_[t][i][v] <= h_final + BIG_M * (1 - x_bar_[t][i][a][h]));
                        constraints[constraints.getSize() - 1].setName(string("intervals_final(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(h) + ")").c_str());
                    }
                }
            }
        }
    }

    // demands by each time interval (17)
    for (int v = 0; v < data.get_nb_vertices(); v++)
    {
        for (int h = 0; h < data.get_nb_intervals(); h++)
        {
            IloExpr expr(env);
            for (int t = 0; t < data.get_nb_trains(); t++)
            {
                for (int i = 0; i < data.get_train_max_trips(t); i++)
                {
                    for (auto arc : data.get_vertex_out_arcs(v))
                    {
                        int a = arc.idx;
                        expr += x_bar_[t][i][a][h];
                    }
                }
            }
            constraints.add(expr >= data.get_demands()[v][h]);
            constraints[constraints.getSize() - 1].setName(string("demands(" + to_string(v) + ")(" + to_string(h) + ")").c_str());
        }
    }

    // collision constraints - different directions (18)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int l = 0; l < data.get_nb_trains(); l++)
        {
            if (t != l)
            {
                for (int i = 0; i < data.get_train_max_trips(t); i++)
                {
                    for (int j = 0; j < data.get_train_max_trips(l); j++)
                    {
                        for (auto inc_point : data.get_inc_points())
                        {
                            int k = get<0>(inc_point);
                            int q = get<1>(inc_point);
                            int v = get<2>(inc_point);
                            int a = (get<3>(inc_point)).idx;

                            constraints.add(y_[t][i][v] >= y_[l][j][q] + data.get_distance(a) - BIG_M * (1 - w_[t][i][v][l][j][k]));
                            constraints[constraints.getSize() - 1].setName(string("collisions_diff_directions(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(k) + ")").c_str());
                        }
                    }
                }
            }
        }
    }

    // associate w variable with x variable (19), (20) and (21)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int l = 0; l < data.get_nb_trains(); l++)
        {
            if (t != l)
            {
                for (int i = 0; i < data.get_train_max_trips(t); i++)
                {
                    for (int j = 0; j < data.get_train_max_trips(l); j++)
                    {
                        for (int v = 0; v < data.get_nb_vertices(); v++)
                        {
                            for (int k = 0; k < data.get_nb_vertices(); k++)
                            {
                                if (v == k)
                                    continue;
                                IloExpr expr1(env);
                                for (auto arc : data.get_vertex_out_arcs(v))
                                {
                                    if (!data.is_reversal_arc(arc))
                                    {
                                        int a = arc.idx;
                                        expr1 += x_[t][i][a];
                                    }
                                }
                                IloExpr expr2(env);
                                for (auto arc : data.get_vertex_out_arcs(k))
                                {
                                    if (!data.is_reversal_arc(arc))
                                    {
                                        int a = arc.idx;
                                        expr2 += x_[l][j][a];
                                    }
                                }

                                constraints.add(w_[t][i][v][l][j][k] <= expr1);
                                constraints[constraints.getSize() - 1].setName(string("link_w_and_x1(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(k) + ")").c_str());

                                constraints.add(w_[t][i][v][l][j][k] <= expr2);
                                constraints[constraints.getSize() - 1].setName(string("link_w_and_x2(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(k) + ")").c_str());

                                constraints.add(w_[t][i][v][l][j][k] + w_[l][j][k][t][i][v] >= expr1 + expr2 - 1);
                                constraints[constraints.getSize() - 1].setName(string("link_w_and_x3(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(k) + ")").c_str());
                            }
                        }
                    }
                }
            }
        }
    }

    // collision constraints - same direction (22)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int l = 0; l < data.get_nb_trains(); l++)
        {
            if (t != l)
            {
                for (int i = 0; i < data.get_train_max_trips(t); i++)
                {
                    for (int j = 0; j < data.get_train_max_trips(l); j++)
                    {
                        for (int v = 0; v < data.get_nb_vertices(); v++)
                        {
                            constraints.add(y_bar[t][i][v] >= y_[l][j][v] - BIG_M * (1 - u_[t][i][l][j][v]));
                            constraints[constraints.getSize() - 1].setName(string("collisions_same_direction(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(v) + ")").c_str());
                        }
                    }
                }
            }
        }
    }

    // minimum headway constraints (23)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int l = 0; l < data.get_nb_trains(); l++)
        {
            if (t != l)
            {
                for (int i = 0; i < data.get_train_max_trips(t); i++)
                {
                    for (int j = 0; j < data.get_train_max_trips(l); j++)
                    {
                        for (int v = 0; v < data.get_nb_vertices(); v++)
                        {
                            constraints.add(y_[t][i][v] >= y_[l][j][v] + data.get_alpha() - BIG_M * (1 - u_[t][i][l][j][v]));
                            constraints[constraints.getSize() - 1].setName(string("headway(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(v) + ")").c_str());
                        }
                    }
                }
            }
        }
    }

    // associate u variable with x variable (24), (25) and (26)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int l = 0; l < data.get_nb_trains(); l++)
        {
            if (t != l)
            {
                for (int i = 0; i < data.get_train_max_trips(t); i++)
                {
                    for (int j = 0; j < data.get_train_max_trips(l); j++)
                    {
                        for (int v = 0; v < data.get_nb_vertices(); v++)
                        {
                            IloExpr expr1(env);
                            for (auto arc : data.get_vertex_out_arcs(v))
                            {
                                int a = arc.idx;
                                expr1 += x_[t][i][a];
                            }
                            IloExpr expr2(env);
                            for (auto arc : data.get_vertex_out_arcs(v))
                            {
                                int a = arc.idx;
                                expr2 += x_[l][j][a];
                            }

                            constraints.add(u_[t][i][l][j][v] <= expr1);
                            constraints[constraints.getSize() - 1].setName(string("link_u_and_x1(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(l) + ")(" + to_string(j) + ")").c_str());

                            constraints.add(u_[t][i][l][j][v] <= expr2);
                            constraints[constraints.getSize() - 1].setName(string("link_u_and_x2(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(l) + ")(" + to_string(j) + ")").c_str());

                            constraints.add(u_[t][i][l][j][v] + u_[l][j][t][i][v] >= expr1 + expr2 - 1);
                            constraints[constraints.getSize() - 1].setName(string("link_u_and_x3(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(l) + ")(" + to_string(j) + ")").c_str());
                        }
                    }
                }
            }
        }
    }
}

int Model::extract_solution(Data &data, bool is_final_solution)
{
    IloCplex cplex(env);

    // set parameters
    cplex.setParam(IloCplex::ClockType, 2);
    cplex.setParam(IloCplex::TiLim, 43200); // set time limit of 12 hours
    cplex.setWarning(env.getNullStream());  // silence warnings
    cplex.setParam(IloCplex::Threads, 1);
    cplex.setParam(IloCplex::ParallelMode, 0);

    // extract model and .lp file
    cplex.extract(model);
    cplex.exportModel("cbtu.lp");

    if (!is_final_solution)
    {
        // remove outputs
        cplex.setOut(env.getNullStream());     
        cplex.setError(env.getNullStream());    

        auto start = chrono::steady_clock::now();
        bool solved = cplex.solve();
        auto end = chrono::steady_clock::now();
        
        if (!solved)
            return 0;

        std::chrono::duration<double> time = end-start;
        current_sol.computational_time = (time).count();
        current_sol.obj_value = cplex.getObjValue();
        
        current_sol.gap_value = cplex.getMIPRelativeGap();
        get_value_of_variables(data, cplex, is_final_solution);
        
        if (current_sol.obj_value < best_sol.obj_value)
        {
            // cout << current_sol.obj_value << " < " << best_sol.obj_value << endl;
            // cout << "Found new best solution - Cost " << current_sol.obj_value << endl;
            best_sol.time_found = chrono::steady_clock::now();

            best_sol.obj_value = current_sol.obj_value;
            best_sol.gap_value = current_sol.gap_value;
            best_sol.y_values = current_sol.y_values;
            best_sol.y_bar_values = current_sol.y_bar_values;
            best_sol.lambda_values = current_sol.lambda_values;
        }
    }
    else
    {
        cout << endl << ">> Solving..." << endl;
        auto start = chrono::steady_clock::now();
        cplex.use(new (env) MyIncumbentCallback(env, start));
        cplex.solve();
        auto end = chrono::steady_clock::now();
        std::chrono::duration<double> time = end-start;
        best_sol.computational_time = (time).count();

        cout << cplex.getStatus() << endl;
        if (cplex.getStatus() == IloCplex::Infeasible)
        {
            cerr << "Error: Instance is infeasible\n";
            return 0;
        }

        cout << fixed << setprecision(2) << "    -> Optimal was found = " << MyIncumbentCallback::best_solution_time << endl;

        best_sol.obj_value = cplex.getObjValue();
        best_sol.gap_value = cplex.getMIPRelativeGap();
    }

    get_value_of_variables(data, cplex, is_final_solution);
    get_solution(data, is_final_solution);
    return 1;
}

void Model::get_value_of_variables(Data &data, IloCplex &cplex, bool is_final_solution)
{
    VarValuesMatrix3d y_values;
    VarValuesMatrix3d y_bar_values;
    VarValuesMatrix3d lambda_values;

    vector<vector<int>> map_indexes_y, map_indexes_y_bar, map_indexes_lambda;
    IloNumVarArray y_array = IloNumVarArray(env);
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            for (int v = 0; v < data.get_nb_vertices(); v++)
            {
                // add and map the current variable
                y_array.add(y_[t][i][v]);
                string name = y_[t][i][v].getName();
                y_array[map_indexes_y.size()].setName(name.c_str());
                map_indexes_y.push_back(vector<int>(3));
                map_indexes_y.back()[0] = t;
                map_indexes_y.back()[1] = i;
                map_indexes_y.back()[2] = v;
            }
        }
    }
    IloNumVarArray y_bar_array = IloNumVarArray(env);
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            for (int v = 0; v < data.get_nb_vertices(); v++)
            {
                // add and map the current variable
                y_bar_array.add(y_bar[t][i][v]);
                string name = y_bar[t][i][v].getName();
                y_bar_array[map_indexes_y_bar.size()].setName(name.c_str());
                map_indexes_y_bar.push_back(vector<int>(3));
                map_indexes_y_bar.back()[0] = t;
                map_indexes_y_bar.back()[1] = i;
                map_indexes_y_bar.back()[2] = v;
            }
        }
    }
    IloNumVarArray lambda_array = IloNumVarArray(env);
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                if (data.is_valid_route(t, i, r))
                {
                    // add and map the current variable
                    lambda_array.add(lambda_[t][i][r]);
                    string name = lambda_[t][i][r].getName();
                    lambda_array[map_indexes_lambda.size()].setName(name.c_str());
                    map_indexes_lambda.push_back(vector<int>(3));
                    map_indexes_lambda.back()[0] = t;
                    map_indexes_lambda.back()[1] = i;
                    map_indexes_lambda.back()[2] = r;
                }
            }
        }
    }

    int max_nb_trips = 0;
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        if (data.get_train_max_trips(t) > max_nb_trips)
            max_nb_trips = data.get_train_max_trips(t);
    }
    // get y values
    IloNumArray y_values_array = IloNumArray(env, map_indexes_y.size());
    cplex.getValues(y_values_array, y_array);
    y_values = vector<vector<vector<int>>>(data.get_nb_trains(),
                                           vector<vector<int>>(max_nb_trips,
                                                               vector<int>(data.get_nb_vertices(), 0)));
    for (int i = 0; i < map_indexes_y.size(); i++)
        y_values[map_indexes_y[i][0]][map_indexes_y[i][1]][map_indexes_y[i][2]] += floor(y_values_array[i] + 1e-5);
    y_values_array.end();
    y_array.end();
    // get y bar values
    IloNumArray y_bar_values_array = IloNumArray(env, map_indexes_y_bar.size());
    cplex.getValues(y_bar_values_array, y_bar_array);
    y_bar_values = vector<vector<vector<int>>>(data.get_nb_trains(),
                                               vector<vector<int>>(max_nb_trips,
                                                                   vector<int>(data.get_nb_vertices(), 0)));
    for (int i = 0; i < map_indexes_y_bar.size(); i++)
        y_bar_values[map_indexes_y_bar[i][0]][map_indexes_y_bar[i][1]][map_indexes_y_bar[i][2]] += floor(y_bar_values_array[i] + 1e-5);
    y_bar_values_array.end();
    y_bar_array.end();
    // get lambda values
    IloNumArray lambda_values_array = IloNumArray(env, map_indexes_lambda.size());
    cplex.getValues(lambda_values_array, lambda_array);
    lambda_values = vector<vector<vector<int>>>(data.get_nb_trains(),
                                                vector<vector<int>>(max_nb_trips,
                                                                    vector<int>(data.get_nb_routes(), 0)));
    for (int i = 0; i < map_indexes_lambda.size(); i++)
        lambda_values[map_indexes_lambda[i][0]][map_indexes_lambda[i][1]][map_indexes_lambda[i][2]] += floor(lambda_values_array[i] + 1e-5);
    lambda_values_array.end();
    lambda_array.end();

    // // display y values
    // for (int t = 0; t < data.get_nb_trains(); t++)
    // {
    //     for (int i = 0; i < data.get_train_max_trips(t); i++)
    //     {
    //         for (int v = 0; v < data.get_nb_vertices(); v++)
    //         {
    //             cout << "constraints.add(y_[" << t << "][" << i << "][" << v << "] == " << cplex.getValue(y_[t][i][v]) << ");" << endl;
    //         }
    //     }
    // }

    // display w values
    // for (int t = 0; t < data.get_nb_trains(); t++)
    // {
    //     for (int i = 0; i < data.get_train_max_trips(t); i++)
    //     {
    //         for (int l = 0; l < data.get_nb_trains(); l++)
    //         {
    //             if (t != l)
    //             {
    //                 for (int j = 0; j < data.get_train_max_trips(l); j++)
    //                 {
    //                     for (int v = 0; v < data.get_nb_vertices(); v++)
    //                     {
    //                         for (int k = 0; k < data.get_nb_vertices(); k++)
    //                         {
    //                             if (v != k)
    //                             {
    //                                 if (cplex.getValue(w_[t][i][v][l][j][k] == 1))
    //                                 {
    //                                     cout << "w_[" << t << "][" << i << "][" << v << "][" << l << "][" << j << "][" << k << "] = 1" << endl;
    //                                 }
    //                             }
    //                         }
    //                     }
    //                 }
    //             }
    //         }
    //     }
    // }

    if (!is_final_solution)
    {
        current_sol.y_values = y_values;
        current_sol.y_bar_values = y_bar_values;
        current_sol.lambda_values = lambda_values;
    }
    else
    {
        best_sol.y_values = y_values;
        best_sol.y_bar_values = y_bar_values;
        best_sol.lambda_values = lambda_values;
    }
}

void Model::get_combination (Data &data, vector<vector<int>> &combination)
{
    VarValuesMatrix3d lambda_values = best_sol.lambda_values;

    combination.clear();
    for (int i = 0; i < data.get_nb_trains(); i++)
    {
        vector <int> aux;
        for (int j = 0; j < data.get_train_max_trips(i); j++)
        {
            for (int k = 0; k < data.get_nb_routes(); k++)
            {
                if (lambda_values[i][j][k] == 1)
                {
                    aux.push_back(k);
                    break;
                } 
            }
        }
        combination.push_back(aux);
    }

    cout << endl << "Melhor custo atual: " << best_sol.obj_value << endl;
    cout << "Melhor solução atual:" << endl;
    for (int i = 0; i < combination.size(); i++)
    {
        cout << "Trem " << i+1 << ": ";
        for (int j = 0; j < combination[i].size(); j++)
        {
            cout << combination[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl;

}

void Model::get_solution (Data &data, bool is_final_solution)
{   
    VarValuesMatrix3d y_values;
    VarValuesMatrix3d y_bar_values;
    VarValuesMatrix3d lambda_values;

    int idx_sol = 0;

    ofstream solution_file, solution_script;
    if (!is_final_solution)
    {
        y_values = current_sol.y_values;
        y_bar_values = current_sol.y_bar_values;
        lambda_values = current_sol.lambda_values;
        
        // creates folder to save files about solution
        string full_path =  "combinations/feasible-combinations/" + data.get_instance_name();
        filesystem::create_directory(full_path);

        idx_sol++;
        string full_path_sol = full_path + "/sol" + to_string(idx_sol);
        while(!filesystem::create_directory(full_path_sol))
        {
            idx_sol++;
            full_path_sol = full_path + "/sol" + to_string(idx_sol);
        }

        solution_file.open(full_path_sol + "/timetable.txt", ios::out | ios::trunc);                   // file to register the timetable obtained
        solution_script.open(full_path_sol + "/script-solution.txt", ios::out | ios::trunc);           // file to execute the python script, in order to generate the graphs of the timetable
        
        solution_file << "-> Solution value = " << current_sol.obj_value << " - " << convert_time(current_sol.obj_value) << endl; 
        solution_file << "-> Total time = " << current_sol.computational_time << endl;
    }
    else
    {
        y_values = best_sol.y_values;
        y_bar_values = best_sol.y_bar_values;
        lambda_values = best_sol.lambda_values;

        // create files to register the solution given by the model
        solution_file.open("solutions/timetables/" + data.get_instance_name() + ".txt", ios::out | ios::trunc); // file to register the timetable
        solution_script.open("script-solution.txt", ios::out | ios::trunc);                                     // file to execute the python script, in order to generate the graphs of the timetable

        solution_file << "-> Solution value = " << best_sol.obj_value << " - " << convert_time(best_sol.obj_value) << endl;
        solution_file << "-> Total time = " << best_sol.computational_time << endl;
        solution_file << "-> Gap value = " << best_sol.gap_value << endl << endl;
    }

    solution_script << "num_points " << data.get_nb_points() << endl;
    solution_script << "---" << endl;

    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        solution_file << "=============" << endl
                        << "Train " << t << endl
                        << "=============" << endl;
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                if (data.is_valid_route(t, i, r))
                {
                    if (lambda_values[t][i][r] > 0)
                    {
                        solution_file << "> Trip " << i << endl;
                        int departure, arrival;
                        for (auto arc : data.get_route_arcs(r))
                        {
                            departure = arc.out;
                            arrival = arc.inc;

                            solution_file << "   " << departure << "(time " << y_values[t][i][departure] << " - " << convert_time(y_values[t][i][departure]) << ")"
                                            << "(time " << y_bar_values[t][i][arrival] << " - " << convert_time(y_bar_values[t][i][arrival])
                                            << ") -> ";

                            solution_script << t << "," << i << "," << data.get_vertex_point(departure) << ": " << y_values[t][i][departure] << " -> ";
                            solution_script << t << "," << i << "," << data.get_vertex_point(arrival) << ": " << (y_values[t][i][departure] + data.get_distance(arc.idx)) << " // ";
                        }
                        solution_file << "   " << arrival << endl;
                    }
                }
            }
            solution_script << endl;
        }
        solution_script << endl;
    }
    solution_file.close();
    solution_script.close();

    // display solution on terminal if it's the final solution
    if (is_final_solution)
    {
        cout << endl << ">> Printing some results..." << endl << fixed << setprecision(2);
        cout << "    -> Solution value = " << best_sol.obj_value << " - " << convert_time(best_sol.obj_value) << endl;
        cout << "    -> Total time = " << best_sol.computational_time << endl;
        cout << "    -> Gap value = " << best_sol.gap_value << endl << endl;

        for (int t = 0; t < data.get_nb_trains(); t++)
        {
            cout << "=============" << endl
                 << "Train " << t << endl
                 << "=============" << endl;
            for (int i = 0; i < data.get_train_max_trips(t); i++)
            {
                for (int r = 0; r < data.get_nb_routes(); r++)
                {
                    if (data.is_valid_route(t, i, r))
                    {
                        if (lambda_values[t][i][r] > 0)
                        {
                            cout << "> Trip " << i << endl;
                            int departure, arrival;
    
                            for (auto arc : data.get_route_arcs(r))
                            {
                                departure = arc.out;
                                arrival = arc.inc;
    
                                cout << "   " << departure << "(time " << y_values[t][i][departure] << " - " << convert_time(y_values[t][i][departure]) << ")"
                                     << "(time " << y_bar_values[t][i][arrival] << " - " << convert_time(y_bar_values[t][i][arrival])
                                     << ") -> ";
                            }
                            cout << arrival << endl;
                        }
                    }
                }
            }
        }

        // get_graph(data, idx_sol);
    }
}

void Model::get_graph (Data &data, int idx_sol)
{
    // calls python script to generate graph of the solution
    string command = "python3 ";
    string file_name = "script.py ";
    string instance_name = "\"" + data.get_instance_name() + "\"";
    command += (file_name + instance_name + " " + to_string(idx_sol));
    system(command.c_str());
}

string Model::convert_time(int seconds)
{
    int hours = 0, minutes = 0;
    while (seconds >= 3600)
    {
        int time = seconds / 3600;
        seconds = seconds % 3600;

        hours += time;
    }
    while (seconds >= 60)
    {
        int time = seconds / 60;
        seconds = seconds % 60;

        minutes += time;
    }

    if (hours < 10)
    {
        if (seconds < 10)
        {
            if (minutes < 10)
                return string("0" + to_string(hours) + ":0" + to_string(minutes) + ":0" + to_string(seconds));
            else
                return string("0" + to_string(hours) + ":" + to_string(minutes) + ":0" + to_string(seconds));
        }
        else
        {
            if (minutes < 10)
                return string("0" + to_string(hours) + ":0" + to_string(minutes) + ":" + to_string(seconds));
            else
                return string("0" + to_string(hours) + ":" + to_string(minutes) + ":" + to_string(seconds));
        }
    }
    else
    {
        if (seconds < 10)
        {
            if (minutes < 10)
                return string(to_string(hours) + ":0" + to_string(minutes) + ":0" + to_string(seconds));
            else
                return string(to_string(hours) + ":" + to_string(minutes) + ":0" + to_string(seconds));
        }
        else
        {
            if (minutes < 10)
                return string(to_string(hours) + ":0" + to_string(minutes) + ":" + to_string(seconds));
            else
                return string(to_string(hours) + ":" + to_string(minutes) + ":" + to_string(seconds));
        }
    }
}
