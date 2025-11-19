#include "Model-OR-Tools.hpp"

using namespace operations_research;
using namespace std;

void ModelORTools::initialize(Data &data, int threads)
{
    // set maximum number of threads to be used
    nb_threads = threads;

    // create the linear solver
    solver = std::unique_ptr<MPSolver>(MPSolver::CreateSolver("HIGHS"));
    if (!solver)
    {
        cerr << "Warning: Could not create solver" << endl;
        return;
    }
}

void ModelORTools::create_full_model(Data &data)
{
    cout << endl << "Creating model..." << endl;
    // create decision variables
    add_variables(data);
    // create objective function
    objective = solver->MutableObjective();
    objective->SetCoefficient(z_, 1);
    objective->SetMinimization();
    // create constraints
    add_constraints(data);
}

void ModelORTools::create_model_with_routes_constraints(Data &data, std::vector<std::vector<int>> &routes_of_trains)
{
    // create decision variables
    add_variables(data);
    // create objective function
    objective = solver->MutableObjective();
    objective->SetCoefficient(z_, 1);
    objective->SetMinimization();
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
                        // make sure that route exists for the first trip
                        if (data.is_valid_route(t, i, r))
                        {
                            // lambda_[t][i][r] == 0
                            MPConstraint* c_route = solver->MakeRowConstraint(0, 0);
                            c_route->SetCoefficient(lambda_[t][i][r], 1);
                        }
                    }
                }
                else if (current[i] != -1)
                {
                    if (data.is_valid_route(t, i, current[i]))
                    {
                        // variable is 1 if route is completed
                        // lambda_[t][i][current[i]] == 1
                        MPConstraint* c_route = solver->MakeRowConstraint(1, 1);
                        c_route->SetCoefficient(lambda_[t][i][current[i]], 1);
                    }
                }
            }
        }
    }
}

void ModelORTools::add_variables(Data &data)
{
    // create variable x - specifies whether train t on trip i uses arc a
    x_.resize(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        x_[t].resize(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            x_[t][i].resize(data.get_nb_arcs());
            for (int a = 0; a < data.get_nb_arcs(); a++)
            {
                string name = "x(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(a) + ")";
                x_[t][i][a] = solver->MakeBoolVar(name.c_str());
            }
        }
    }

    // create variable x bar - specifies whether train t on trip i uses arc a on time interval h
    x_bar_.resize(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        x_bar_[t].resize(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            x_bar_[t][i].resize(data.get_nb_arcs());
            for (int a = 0; a < data.get_nb_arcs(); a++)
            {
                x_bar_[t][i][a].resize(data.get_nb_intervals());
                for (int h = 0; h < data.get_nb_intervals(); h++)
                {
                    string name = "x_bar(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(a) + ")(" + to_string(h) + ")";
                    x_bar_[t][i][a][h] = solver->MakeBoolVar(name.c_str());
                }
            }
        }
    }

    // create variable y - departure time at a vertex
    y_.resize(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        y_[t].resize(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            y_[t][i].resize(data.get_nb_vertices());
            for (int v = 0; v < data.get_nb_vertices(); v++)
            {
                string name = "y(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")";
                y_[t][i][v] = solver->MakeNumVar(0, solver->infinity(), name.c_str());
            }
        }
    }

    // create variable y bar - arrival time at a vertex
    y_bar_.resize(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        y_bar_[t].resize(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            y_bar_[t][i].resize(data.get_nb_vertices());
            for (int v = 0; v < data.get_nb_vertices(); v++)
            {
                string name = "y_bar(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")";
                y_bar_[t][i][v] = solver->MakeNumVar(0, solver->infinity(), name.c_str());
            }
        }
    }

    // create variable lambda - specifies whether train t on trip i uses route r
    lambda_.resize(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        lambda_[t].resize(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            lambda_[t][i].resize(data.get_nb_routes());
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                string name = "lambda(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(r) + ")";
                lambda_[t][i][r] = solver->MakeBoolVar(name.c_str());
            }
        }
    }

    // create variable w - specifies whether train t departs from vertex v after train l departs from vertex k
    w_.resize(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        w_[t].resize(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            w_[t][i].resize(data.get_nb_vertices());
            for (int v = 0; v < data.get_nb_vertices(); v++)
            {
                w_[t][i][v].resize(data.get_nb_trains());
                for (int l = 0; l < data.get_nb_trains(); l++)
                {
                    if (t != l)
                    {
                        w_[t][i][v][l].resize(data.get_train_max_trips(l));
                        for (int j = 0; j < data.get_train_max_trips(l); j++)
                        {
                            w_[t][i][v][l][j].resize(data.get_nb_vertices());
                            for (int k = 0; k < data.get_nb_vertices(); k++)
                            {
                                for (auto inc_point : data.get_inc_points())
                                {
                                    int k_aux = get<0>(inc_point);
                                    int v_aux = get<2>(inc_point);

                                    if (v_aux == v && k_aux == k)
                                    {
                                        string name = "w(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(k) + ")";
                                        w_[t][i][v][l][j][k] = solver->MakeBoolVar(name.c_str());
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // create variable u - specifies whether train t departs from vertex v after train l
    u_.resize(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        u_[t].resize(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            u_[t][i].resize(data.get_nb_trains());
            for (int l = 0; l < data.get_nb_trains(); l++)
            {
                if (t != l)
                {
                    u_[t][i][l].resize(data.get_train_max_trips(l));
                    for (int j = 0; j < data.get_train_max_trips(l); j++)
                    {
                        u_[t][i][l][j].resize(data.get_nb_vertices());
                        for (int v = 0; v < data.get_nb_vertices(); v++)
                        {
                            string name = "u(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(v) + ")";
                            u_[t][i][l][j][v] = solver->MakeBoolVar(name.c_str());
                        }
                    }
                }
            }
        }
    }

    // create variable z - objective function
    z_ = solver->MakeNumVar(0, solver->infinity(), "z");
}

void ModelORTools::add_constraints(Data &data)
{
    // constraints to get value of z (2)
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
                            // z_ >= y_[t][i][v] - y_[l][j][v]

                            int v = data.get_point_vertices(p)[0];

                            string name = "max_gap_between_departures_upper(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(v) + ")";
                            MPConstraint* c_2 = solver->MakeRowConstraint(-solver->infinity(), 0, name.c_str());
                            c_2->SetCoefficient(z_, -1);
                            c_2->SetCoefficient(y_[t][i][v], 1);
                            c_2->SetCoefficient(y_[l][j][v], -1);

                            v = data.get_point_vertices(p)[1];

                            name = "max_gap_between_departures_lower(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(v) + ")";
                            c_2 = solver->MakeRowConstraint(-solver->infinity(), 0, name.c_str());
                            c_2->SetCoefficient(z_, -1);
                            c_2->SetCoefficient(y_[t][i][v], 1);
                            c_2->SetCoefficient(y_[l][j][v], -1);
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
                // x_[t][i][a] == expr

                string name = "associate_x_and_x_bar(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(a) + ")";
                MPConstraint* c_3 = solver->MakeRowConstraint(0, 0, name.c_str());
                c_3->SetCoefficient(x_[t][i][a], -1);
                for (int h = 0; h < data.get_nb_intervals(); h++)
                {
                    c_3->SetCoefficient(x_bar_[t][i][a][h], 1);
                }
            }
        }
    }

    // constraints to associate routes with arcs (lambda variables with x variables) (4)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            for (int a = 0; a < data.get_nb_arcs(); a++)
            {
                // x_[t][i][a] == expr

                string name = "associate_routes_with_arcs(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(a) + ")";
                MPConstraint* c_4 = solver->MakeRowConstraint(0, 0, name.c_str());
                c_4->SetCoefficient(x_[t][i][a], -1);
                for (int r = 0; r < data.get_nb_routes(); r++)
                {
                    if (data.is_valid_route(t, i, r) && data.arc_belongs_to_route(r, a))
                    {
                        c_4->SetCoefficient(lambda_[t][i][r], 1);
                    }
                }
            }
        }
    }

    // each trip completed must be associated with at most one route (5)
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            // expr <= 1

            string name = "associate_trip_with_route(" + to_string(t) + ")(" + to_string(i) + ")";
            MPConstraint* c_5 = solver->MakeRowConstraint(-solver->infinity(), 1, name.c_str());
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                if (data.is_valid_route(t, i, r))
                {
                    c_5->SetCoefficient(lambda_[t][i][r], 1);
                }
            }
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

                                // lambda_[t][i][r1] + lambda_[t][i - 1][r2] <= 1
                                string name = "incompatible_routes(" + to_string(t) + ")(" + to_string(i - 1) + ")(" + to_string(i) + ")(" + to_string(r1) + ")(" + to_string(r2) + ")";
                                MPConstraint* c_6 = solver->MakeRowConstraint(-solver->infinity(), 1, name.c_str());
                                c_6->SetCoefficient(lambda_[t][i][r1], 1);
                                c_6->SetCoefficient(lambda_[t][i - 1][r2], 1);
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
            // expr1 <= expr2

            string name = "subseq_trips(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(i - 1) + ")";
            MPConstraint* c_7 = solver->MakeRowConstraint(0, solver->infinity(), name.c_str());
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                if (data.is_valid_route(t, i, r))
                {
                    c_7->SetCoefficient(lambda_[t][i][r], -1);
                }
            }
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                if (data.is_valid_route(t, i - 1, r))
                {
                    c_7->SetCoefficient(lambda_[t][i - 1][r], 1);
                }
            }
        }
    }

    // constraints to connect trips of the same train (trip can only start after the previous one has ended) (8)
    for (int t = 0; t < data.get_nb_trains(); ++t)
    {
        for (int i = 1; i < data.get_train_max_trips(t); ++i)
        {
            for (int v = 0; v < data.get_nb_vertices(); ++v)
            {
                for (int k = 0; k < data.get_nb_vertices(); ++k)
                {
                    if (data.can_be_adjacent_in_consecutive_trips(v, k))
                    {
                        // y_[t][i][k] >= y_bar[t][i - 1][v] - BIG_M * (2 - expr1 - expr2))
                        // 0 >= y_bar[t][i - 1][v] - BIG_M * (2 - expr1 - expr2)) - y_[t][i][k] >= -inf
                        // 0 >= y_bar[t][i - 1][v] - BIG_M * 2 + BIG_M * expr1 + + BIG_M * expr2 - y_[t][i][k] >= -inf
                        // 0 >= y_bar[t][i - 1][v] - BIG_M * 2 + BIG_M * (expr1 + expr2) - y_[t][i][k] >= -inf
                        // BIG_M * 2 >= y_bar[t][i - 1][v] + BIG_M * (expr1 + expr2) - y_[t][i][k] >= -inf

                        string name = "connect_trips(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(k) + ")";
                        MPConstraint* c_8 = solver->MakeRowConstraint(-solver->infinity(), 2*BIG_M, name.c_str());
                        c_8->SetCoefficient(y_bar_[t][i - 1][v], 1);
                        c_8->SetCoefficient(y_[t][i][k], -1);
                        for (int r = 0; r < data.get_nb_routes(); r++)
                        {
                            if (data.finish_at_vertex(r, v) && data.is_valid_route(t, i - 1, r))
                            {
                                c_8->SetCoefficient(lambda_[t][i - 1][r], BIG_M);
                            }
                        }
                        for (int r = 0; r < data.get_nb_routes(); r++)
                        {
                            if (data.start_at_vertex(r, k) && data.is_valid_route(t, i, r))
                            {
                                c_8->SetCoefficient(lambda_[t][i][r], BIG_M);
                            }
                        }
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

                        // y_bar[t][i][k] >= y_[t][i][v] + data.get_distance(a) - BIG_M * (1 - lambda_[t][i][r])
                        // 0 >= y_[t][i][v] + data.get_distance(a) - BIG_M * (1 - lambda_[t][i][r]) - y_bar[t][i][k] >= -inf
                        // -data.get_distance(a) >= y_[t][i][v] - BIG_M + BIG_M * lambda_[t][i][r] - y_bar[t][i][k] >= -inf
                        // -data.get_distance(a) + BIG_M >= y_[t][i][v] + BIG_M * lambda_[t][i][r] - y_bar[t][i][k] >= -inf

                        string name = "traveling_time1(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(r) + ")";
                        MPConstraint* c_9 = solver->MakeRowConstraint(-solver->infinity(), -data.get_distance(a) + BIG_M, name.c_str());
                        c_9->SetCoefficient(y_[t][i][v], 1);
                        c_9->SetCoefficient(lambda_[t][i][r], BIG_M);
                        c_9->SetCoefficient(y_bar_[t][i][k], -1);

                        // y_bar[t][i][k] <= y_[t][i][v] + data.get_distance(a) + BIG_M * (1 - lambda_[t][i][r])
                        // 0 <= y_[t][i][v] + data.get_distance(a) + BIG_M * (1 - lambda_[t][i][r]) - y_bar[t][i][k] <= inf
                        // -data.get_distance(a) <= y_[t][i][v] + BIG_M - BIG_M * lambda_[t][i][r] - y_bar[t][i][k] <= inf
                        // -data.get_distance(a) - BIG_M <= y_[t][i][v] - BIG_M * lambda_[t][i][r] - y_bar[t][i][k] <= inf

                        name = "traveling_time2(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(r) + ")";
                        MPConstraint* c_10 = solver->MakeRowConstraint(-data.get_distance(a) - BIG_M, solver->infinity(), name.c_str());
                        c_10->SetCoefficient(y_[t][i][v], 1);
                        c_10->SetCoefficient(lambda_[t][i][r], -BIG_M);
                        c_10->SetCoefficient(y_bar_[t][i][k], -1);
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

                        // y_[t][i][k] >= y_[t][i][v] + data.get_distance_and_service_min(a) - BIG_M * (1 - lambda_[t][i][r])
                        // 0 >= y_[t][i][v] + data.get_distance_and_service_min(a) - BIG_M * (1 - lambda_[t][i][r]) - y_[t][i][k] >= -inf
                        // -data.get_distance_and_service_min(a) >= y_[t][i][v] - BIG_M + BIG_M * lambda_[t][i][r] - y_[t][i][k] >= -inf
                        // -data.get_distance_and_service_min(a) + BIG_M >= y_[t][i][v] + BIG_M * lambda_[t][i][r] - y_[t][i][k] >= -inf

                        // minimum service time
                        string name = "service_time_min(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(r) + ")";
                        MPConstraint* c_11 = solver->MakeRowConstraint(-solver->infinity(), -data.get_distance_and_service_min(a) + BIG_M, name.c_str());
                        c_11->SetCoefficient(y_[t][i][v], 1);
                        c_11->SetCoefficient(lambda_[t][i][r], BIG_M);
                        c_11->SetCoefficient(y_[t][i][k], -1);

                        // y_[t][i][k] <= y_[t][i][v] + data.get_distance_and_service_max(a) + BIG_M * (1 - lambda_[t][i][r])
                        // 0 <= y_[t][i][v] + data.get_distance_and_service_max(a) + BIG_M * (1 - lambda_[t][i][r]) - y_[t][i][k] <= inf
                        // -data.get_distance_and_service_max(a) <= y_[t][i][v] + BIG_M - BIG_M * lambda_[t][i][r] - y_[t][i][k] <= inf
                        // -data.get_distance_and_service_max(a) - BIG_M <= y_[t][i][v] - BIG_M * lambda_[t][i][r] - y_[t][i][k] <= inf

                        // maximum service time
                        name = "service_time_max(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(r) + ")";
                        MPConstraint* c_12 = solver->MakeRowConstraint(-data.get_distance_and_service_max(a) - BIG_M, solver->infinity(), name.c_str());
                        c_12->SetCoefficient(y_[t][i][v], 1);
                        c_12->SetCoefficient(lambda_[t][i][r], -BIG_M);
                        c_12->SetCoefficient(y_[t][i][k], -1);
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
                // y_[t][i][v] <= data.get_max_time() * expr
                // 0 <= data.get_max_time() * expr - y_[t][i][v] <= inf

                // max time for outgoing vertices
                string name = "max_time_out(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")";
                MPConstraint* c_13 = solver->MakeRowConstraint(0, solver->infinity(), name.c_str());
                c_13->SetCoefficient(y_[t][i][v], -1);
                for (auto arc : data.get_vertex_out_arcs(v))
                {
                    // outcoming vertices
                    int a = arc.idx;
                    c_13->SetCoefficient(x_[t][i][a], data.get_max_time());
                }

                // max time for incoming vertices
                name = "max_time_inc(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")";
                MPConstraint* c_14 = solver->MakeRowConstraint(0, solver->infinity(), name.c_str());
                c_14->SetCoefficient(y_bar_[t][i][v], -1);
                for (auto arc : data.get_vertex_inc_arcs(v))
                {
                    // incoming vertices
                    int a = arc.idx;
                    c_14->SetCoefficient(x_[t][i][a], data.get_max_time());
                }
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
                        // y_[t][i][v] >= h_start - BIG_M * (1 - x_bar_[t][i][a][h])
                        // 0 >= h_start - BIG_M * (1 - x_bar_[t][i][a][h]) - y_[t][i][v] >= -inf
                        // 0 >= h_start - BIG_M + BIG_M * x_bar_[t][i][a][h] - y_[t][i][v] >= -inf
                        // -h_start + BIG_M >= BIG_M * x_bar_[t][i][a][h] - y_[t][i][v] >= -inf

                        int a = arc.idx;
                        string name = "intervals_start(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(h) + ")";
                        MPConstraint* c_15 = solver->MakeRowConstraint(-solver->infinity(), -h_start + BIG_M, name.c_str());
                        c_15->SetCoefficient(y_[t][i][v], -1);
                        c_15->SetCoefficient(x_bar_[t][i][a][h], BIG_M);

                        // y_[t][i][v] <= h_final + BIG_M * (1 - x_bar_[t][i][a][h])
                        // 0 <= h_final + BIG_M * (1 - x_bar_[t][i][a][h]) - y_[t][i][v] <= inf
                        // 0 <= h_final + BIG_M - BIG_M * x_bar_[t][i][a][h] - y_[t][i][v] <= inf
                        // -h_final - BIG_M <= BIG_M * x_bar_[t][i][a][h] - y_[t][i][v] <= inf

                        name = "intervals_final(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(h) + ")";
                        MPConstraint* c_16 = solver->MakeRowConstraint(-h_final - BIG_M, solver->infinity(), name.c_str());
                        c_16->SetCoefficient(y_[t][i][v], -1);
                        c_16->SetCoefficient(x_bar_[t][i][a][h], BIG_M);
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

            // expr >= data.get_demands()[v][h]
            // inf >= expr >= data.get_demands()[v][h]

            string name = "demands(" + to_string(v) + ")(" + to_string(h) + ")";
            MPConstraint* c_17 = solver->MakeRowConstraint(data.get_demands()[v][h], solver->infinity(), name.c_str());
            for (int t = 0; t < data.get_nb_trains(); t++)
            {
                for (int i = 0; i < data.get_train_max_trips(t); i++)
                {
                    for (auto arc : data.get_vertex_out_arcs(v))
                    {
                        int a = arc.idx;
                        c_17->SetCoefficient(x_bar_[t][i][a][h], 1);
                    }
                }
            }
        }
    }

    // associate w variable with x variable (18), (19) and (20)
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
                            int v = get<2>(inc_point);

                            if (v == k)
                                continue;

                            // w_[t][i][v][l][j][k] <= expr1
                            // 0 <= expr1 - w_[t][i][v][l][j][k] <= inf
    
                            string name = "link_w_and_x1(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(k) + ")";
                            MPConstraint* c_18 = solver->MakeRowConstraint(0, solver->infinity(), name.c_str());
                            c_18->SetCoefficient(w_[t][i][v][l][j][k], -1);
                            for (auto arc : data.get_vertex_out_arcs(v))
                            {
                                if (!data.is_reversal_arc(arc))
                                {
                                    int a = arc.idx;
                                    c_18->SetCoefficient(x_[t][i][a], 1);
                                }
                            }

                            // w_[t][i][v][l][j][k] <= expr2
                            // 0 <= expr2 - w_[t][i][v][l][j][k] <= inf
                            // expr2 >= w_[t][i][v][l][j][k]
                            name = "link_w_and_x2(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(k) + ")";
                            MPConstraint* c_19 = solver->MakeRowConstraint(0, solver->infinity(), name.c_str());
                            c_19->SetCoefficient(w_[t][i][v][l][j][k], -1);
                            for (auto arc : data.get_vertex_out_arcs(k))
                            {
                                if (!data.is_reversal_arc(arc))
                                {
                                    int a = arc.idx;
                                    c_19->SetCoefficient(x_[l][j][a], 1);
                                }
                            }

                            // w_[t][i][v][l][j][k] + w_[l][j][k][t][i][v] >= expr1 + expr2 - 1
                            // 1 >= expr1 + expr2 - w_[t][i][v][l][j][k] - w_[l][j][k][t][i][v] >= -inf

                            name = "link_w_and_x3(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(k) + ")";
                            MPConstraint* c_20 = solver->MakeRowConstraint(-solver->infinity(), 1, name.c_str());
                            c_20->SetCoefficient(w_[t][i][v][l][j][k], -1);
                            c_20->SetCoefficient(w_[l][j][k][t][i][v], -1);
                            for (auto arc : data.get_vertex_out_arcs(v))
                            {
                                if (!data.is_reversal_arc(arc))
                                {
                                    int a = arc.idx;
                                    c_20->SetCoefficient(x_[t][i][a], 1);
                                }
                            }
                            for (auto arc : data.get_vertex_out_arcs(k))
                            {
                                if (!data.is_reversal_arc(arc))
                                {
                                    int a = arc.idx;
                                    c_20->SetCoefficient(x_[l][j][a], 1);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // associate u variable with x variable (21), (22) and (23)
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

                            // u_[t][i][l][j][v] <= expr1
                            // 0 <= expr1 - u_[t][i][l][j][v] <= inf

                            string name = "link_u_and_x1(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(l) + ")(" + to_string(j) + ")";
                            MPConstraint* c_21 = solver->MakeRowConstraint(0, solver->infinity(), name.c_str());
                            c_21->SetCoefficient(u_[t][i][l][j][v], -1);
                            for (auto arc : data.get_vertex_out_arcs(v))
                            {
                                int a = arc.idx;
                                c_21->SetCoefficient(x_[t][i][a], 1);
                            }

                            // u_[t][i][l][j][v] <= expr2
                            // 0 <= expr2 - u_[t][i][l][j][v] <= inf

                            name = "link_u_and_x2(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(l) + ")(" + to_string(j) + ")";
                            MPConstraint* c_22 = solver->MakeRowConstraint(0, solver->infinity(), name.c_str());
                            c_22->SetCoefficient(u_[t][i][l][j][v], -1);
                            for (auto arc : data.get_vertex_out_arcs(v))
                            {
                                int a = arc.idx;
                                c_22->SetCoefficient(x_[l][j][a], 1);
                            }

                            // u_[t][i][l][j][v] + u_[l][j][t][i][v] >= expr1 + expr2 - 1
                            // 1 >= expr1 + expr2 - u_[t][i][l][j][v] - u_[l][j][t][i][v] >= -inf
                            name = "link_u_and_x3(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(l) + ")(" + to_string(j) + ")";
                            MPConstraint* c_23 = solver->MakeRowConstraint(-solver->infinity(), 1, name.c_str());
                            c_23->SetCoefficient(u_[t][i][l][j][v], -1);
                            c_23->SetCoefficient(u_[l][j][t][i][v], -1);
                            for (auto arc : data.get_vertex_out_arcs(v))
                            {
                                int a = arc.idx;
                                c_23->SetCoefficient(x_[t][i][a], 1);
                                c_23->SetCoefficient(x_[l][j][a], 1);
                            }
                        }
                    }
                }
            }
        }
    }

    // collision constraints - different directions (24)
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

                            // y_[t][i][v] >= y_[l][j][q] + data.get_distance(a) - BIG_M * (1 - w_[t][i][v][l][j][k])
                            // 0 >= y_[l][j][q] + data.get_distance(a) - BIG_M * (1 - w_[t][i][v][l][j][k]) - y_[t][i][v] >= -inf
                            // 0 >= y_[l][j][q] + data.get_distance(a) - BIG_M + BIG_M * w_[t][i][v][l][j][k] - y_[t][i][v] >= -inf
                            // -data.get_distance(a) + BIG_M >= y_[l][j][q] + BIG_M * w_[t][i][v][l][j][k] - y_[t][i][v] >= -inf

                            string name = "collisions_diff_directions(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(v) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(k) + ")";
                            MPConstraint* c_24 = solver->MakeRowConstraint(-solver->infinity(), -data.get_distance(a) + BIG_M, name.c_str());
                            c_24->SetCoefficient(y_[l][j][q], 1);
                            c_24->SetCoefficient(w_[t][i][v][l][j][k], BIG_M);
                            c_24->SetCoefficient(y_[t][i][v], -1);
                        }
                    }
                }
            }
        }
    }

    // collision constraints - same direction (25)
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

                            // y_bar[t][i][v] >= y_[l][j][v] - BIG_M * (1 - u_[t][i][l][j][v])
                            // 0 >= y_[l][j][v] - BIG_M * (1 - u_[t][i][l][j][v]) - y_bar[t][i][v] >= -inf
                            // 0 >= y_[l][j][v] - BIG_M + BIG_M * u_[t][i][l][j][v] - y_bar[t][i][v] >= -inf
                            // BIG_M >= y_[l][j][v] + BIG_M * u_[t][i][l][j][v] - y_bar[t][i][v] >= -inf

                            string name = "collisions_same_direction(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(v) + ")";
                            MPConstraint* c_25 = solver->MakeRowConstraint(-solver->infinity(), BIG_M, name.c_str());
                            c_25->SetCoefficient(y_[l][j][v], 1);
                            c_25->SetCoefficient(u_[t][i][l][j][v], BIG_M);
                            c_25->SetCoefficient(y_bar_[t][i][v], -1);
                        }
                    }
                }
            }
        }
    }

    // minimum headway constraints (26)
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

                            // y_[t][i][v] >= y_[l][j][v] + data.get_alpha() - BIG_M * (1 - u_[t][i][l][j][v])
                            // 0 >= y_[l][j][v] + data.get_alpha() - BIG_M * (1 - u_[t][i][l][j][v]) - y_[t][i][v] >= -inf
                            // 0 >= y_[l][j][v] + data.get_alpha() - BIG_M + BIG_M * u_[t][i][l][j][v] - y_[t][i][v] >= -inf
                            // -data.get_alpha() + BIG_M >= y_[l][j][v] + BIG_M * u_[t][i][l][j][v] - y_[t][i][v] >= -inf

                            string name = "headway(" + to_string(t) + ")(" + to_string(i) + ")(" + to_string(l) + ")(" + to_string(j) + ")(" + to_string(v) + ")";
                            MPConstraint* c_26 = solver->MakeRowConstraint(-solver->infinity(), -data.get_alpha() + BIG_M, name.c_str());
                            c_26->SetCoefficient(y_[l][j][v], 1);
                            c_26->SetCoefficient(u_[t][i][l][j][v], BIG_M);
                            c_26->SetCoefficient(y_[t][i][v], -1);
                        }
                    }
                }
            }
        }
    }
}

void ModelORTools::get_value_of_variables(Data &data)
{
    // get x values
    current_sol.x_values_ = VarValuesMatrix3d(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        current_sol.x_values_[t] = VarValuesMatrix2d(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            current_sol.x_values_[t][i] = vector<int>(data.get_nb_arcs());
            for (int a = 0; a < data.get_nb_arcs(); a++)
            {
                current_sol.x_values_[t][i][a] = x_[t][i][a]->solution_value();
            }
        }
    }

    // get x bar values  
    current_sol.x_bar_values_ = VarValuesMatrix4d(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        current_sol.x_bar_values_[t] = VarValuesMatrix3d(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            current_sol.x_bar_values_[t][i] = VarValuesMatrix2d(data.get_nb_arcs());
            for (int a = 0; a < data.get_nb_arcs(); a++)
            {
                current_sol.x_bar_values_[t][i][a] = vector<int>(data.get_nb_intervals());
                for (int h = 0; h < data.get_nb_intervals(); h++)
                {
                    current_sol.x_bar_values_[t][i][a][h] = x_bar_[t][i][a][h]->solution_value();
                }
            }
        }
    }

    // get y values
    current_sol.y_values_ = VarValuesMatrix3d(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        current_sol.y_values_[t] = VarValuesMatrix2d(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            current_sol.y_values_[t][i] = vector<int>(data.get_nb_vertices());
            for (int v = 0; v < data.get_nb_vertices(); v++)
            {
                current_sol.y_values_[t][i][v] = y_[t][i][v]->solution_value();
            }
        }
    }

    // get y bar values
    current_sol.y_bar_values_ = VarValuesMatrix3d(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        current_sol.y_bar_values_[t] = VarValuesMatrix2d(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            current_sol.y_bar_values_[t][i] = vector<int>(data.get_nb_vertices());
            for (int v = 0; v < data.get_nb_vertices(); v++)
            {
                current_sol.y_bar_values_[t][i][v] = y_bar_[t][i][v]->solution_value();
            }
        }
    }

    // get lambda values
    current_sol.lambda_values_ = VarValuesMatrix3d(data.get_nb_trains());
    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        current_sol.lambda_values_[t] = VarValuesMatrix2d(data.get_train_max_trips(t));
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            current_sol.lambda_values_[t][i] = vector<int>(data.get_nb_routes());
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                current_sol.lambda_values_[t][i][r] = lambda_[t][i][r]->solution_value();
            }
        }
    }

    // get w values
    // get u values
}

int ModelORTools::execute_solver_for_full_model(Data &data) // return 1 if the solver found a solution, 0 otherwise
{
    // setting parameters
    std::string params = R"(
        time_limit = 600
        threads = 1
        log_to_console = true
        output_flag = true
        log_file = highs_log.txt
    )";
    // solver->SetSolverSpecificParametersAsString(params);
    // solver->SetSolverSpecificParametersAsString("log_to_console=true");
    // solver->SetSolverSpecificParametersAsString("log_file=highs_log.txt");
    // solver->EnableOutput();

    auto start = chrono::steady_clock::now();
    const MPSolver::ResultStatus result_status = solver->Solve();
    auto end = chrono::steady_clock::now();

    current_sol.computational_time = end-start;
    current_sol.obj_value = objective->Value();

    cout << "Status: " << result_status << endl;
    if (result_status != MPSolver::OPTIMAL)
    {
        cout << "Optimal solution was not proven." << endl;
        if (result_status != MPSolver::FEASIBLE)
        {
            cout << "The solver could not solve the problem." << endl;
            return 0;
        }
        current_sol.proven_optimal = false;
    }
    else
    {
        current_sol.proven_optimal = true;
    }
    return 1;
}

int ModelORTools::execute_solver_for_combination(Data &data, int best_bound, int time_limit_for_combination, string method)
{
    // setting parameters
    std::string params = R"(
        time_limit = 600
        threads = 1
        log_to_console = true
        output_flag = true
        log_file = highs_log.txt
    )";

    auto start = chrono::steady_clock::now();
    const MPSolver::ResultStatus result_status = solver->Solve();
    auto end = chrono::steady_clock::now();

    if (result_status != MPSolver::OPTIMAL)
    {
        // a feasible solution was found, but the optimal solution was not proven
        if (result_status != MPSolver::FEASIBLE)
        {
            return 0;
        }
        current_sol.proven_optimal = false;
    }
    else
    {
        current_sol.proven_optimal = true;
    }

    current_sol.feasible = true;

    current_sol.obj_value = objective->Value();
    if (current_sol.obj_value < best_sol.obj_value)
    {
        get_value_of_variables(data);

        current_sol.store_combination(data);
        current_sol.store_max_nb_repeated_route(data);

        current_sol.computational_time = end-start;

        best_sol = current_sol;
    }
    else if (method == "heuristic" && current_sol.obj_value == best_sol.obj_value)
    {
        // apply tie breaker for solutions with the same objective value
        get_value_of_variables(data);

        current_sol.store_combination(data);
        current_sol.store_max_nb_repeated_route(data);

        if (current_sol.max_nb_repeated_routes > best_sol.max_nb_repeated_routes)
        {
            current_sol.computational_time = end-start;

            best_sol = current_sol;
        }
    }
    return 1;
}
