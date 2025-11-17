#include <Combinations.hpp>

using namespace std;

Combinations::Combinations(Data &data, int threads, string method, int time_limit_complete)
{
    // start counting time
    start = chrono::steady_clock::now();

    nb_threads = threads;
    nb_routes = data.get_nb_routes();
    nb_trains = data.get_nb_trains();

    if (method == "enum") // executing enumeration
    {
        time_limit_per_combination = 7200;
        execute_enumeration(data);

        if (proved_optimal)
        {
            cout << "All combination(s) tested!" << endl;

            // get number of feasible solutions
            cout << nb_feasible_combinations << "/" << total_nb_combinations << " were feasible combination(s)! - (" << setprecision(3) << (double(nb_feasible_combinations) / double(total_nb_combinations) ) * 100 << "\%)" << endl;
        }
        else
        {
            proved_optimal = false;
            cout << "\tCannot prove optimality!" << endl;

            if (nb_feasible_combinations == 0)
            {
                cout << "\tNo feasible solution was found..." << endl;
                exit(0);
            }
        }

        // verify whether feasible solution was found
        if (nb_feasible_combinations == 0)
        {
            cout << "\t> Instance is infeasible!" << endl;
            return;
        }
    }
    else if (method == "heuristic") // executing heuristic
    {
        time_limit_per_combination = 2400;
        execute_heuristic(data);
        proved_optimal = false;

        // verify whether feasible solution was found
        if (nb_feasible_combinations == 0)
        {
            cout << "\t> Couldn't find feasible solutions with heuristic method!" << endl;
            return;
        }
    }

    // finish counting time
    end = chrono::steady_clock::now();
    overall_best_sol.computational_time = end-start;

    // get optimal solution
    overall_best_sol.get_solution(data);
}

/*-----------------------------------------------------------------*/
/* ===================== ENUMERATION METHODS ===================== */
/*-----------------------------------------------------------------*/

void Combinations::execute_enumeration(Data &data)
{
    // calculate total number of possible combinations
    calculate_trips_combinations(data);
    total_nb_combinations = 1;
    for (int i = 0; i < trips_combinations.size(); i++)
    {
        total_nb_combinations *= trips_combinations[i].size(); 
    }

    cout << "Starting to test combinations... - Total number of combinations = " << total_nb_combinations << endl;
    execute_all_combinations(data);
}

void Combinations::execute_all_combinations(Data &data)
{
    // variable to assist in displaying progress
    aux_progress = ceil(0.1 * total_nb_combinations);
    if (aux_progress == 0)
        aux_progress = 1;

    // vectors with model objects for each thread
    vector <ModelORTools> models(nb_threads);

    #pragma omp parallel num_threads(nb_threads)
    {
        int thread_id = omp_get_thread_num();
        #pragma omp for schedule(dynamic)
        for (unsigned long long count = 0; count < total_nb_combinations; count++)
        {
            // get current combination
            int idx = count;
            vector<int> indices(nb_trains);
            for (int k = nb_trains - 1; k >= 0; k--)
            {
                indices[k] = idx % trips_combinations[k].size();
                idx /= trips_combinations[k].size();
            }
            vector<vector<int>> current;
            for (int k = 0; k < nb_trains; k++)
            {
                current.push_back(trips_combinations[k][indices[k]]);
            }

            if (is_valid_combination(data, current))
            {
                Model &model_thread = models[thread_id];
                model_thread.initialize(data, 1);
                model_thread.create_model_with_routes_constraints(data, current);
                int feasible = model_thread.execute_solver_for_combination(data, overall_best_sol.obj_value, time_limit_per_combination);
                if (feasible)
                {
                    #pragma omp atomic
                    nb_feasible_combinations++;
                    #pragma omp critical
                    {
                        if (model_thread.best_sol.obj_value < overall_best_sol.obj_value)
                        {
                            overall_best_sol = model_thread.best_sol;
                        }
                        else if (nb_threads > 1 && model_thread.best_sol.obj_value == overall_best_sol.obj_value)
                        {
                            // apply tie breaker when dealing with multiple threads
                            if (model_thread.best_sol.max_nb_repeated_route > overall_best_sol.max_nb_repeated_route)
                            {
                                overall_best_sol = model_thread.best_sol;
                            }
                        }
                    }
                }
            }

            // display progress
            if (counter_solved % aux_progress == 0)
                cout << counter_solved/aux_progress * 10 << "%" << " done - " << counter_solved << "/" << total_nb_combinations << " combination(s) tested! (Thread " << thread_id << ")" << endl;

            // verify time limit
            chrono::time_point<std::chrono::steady_clock> now = chrono::steady_clock::now();
            chrono::duration<double> current_time = now - start;
            if (current_time.count() >= time_limit_complete)
            {
                #pragma omp critical
                {
                    cout << endl << ">> Reached time limit!" << endl;
                    cout << "\tTerminating process!" << endl;
                    if (nb_feasible_combinations == 0)
                    {
                        cout << "\tNo feasible solution was found..." << endl;
                    }
                    else
                    {
                        proved_optimal = false;
                        cout << "\tCannot prove optimality!" << endl;

                        overall_best_sol.computational_time = current_time;
                        overall_best_sol.get_solution(data);
                    }
                    exit(0);
                }
            }
        }
    }

    // -------------------------------------------------

    // // vectors with model objects for each thread
    // vector <Model> models(nb_threads);

    // #pragma omp parallel num_threads(nb_threads)
    // {
    //     int thread_id = omp_get_thread_num();
    //     models[thread_id].best_sol.push_back(models[thread_id].current_sol);
    //     // models[thread_id].initialize(data, nb_threads);

    //     #pragma omp for schedule(dynamic)
    //     for (unsigned long long count = 0; count < total_nb_combinations; count++)
    //     {
    //         // cout << "Testing combination " << count << "/" << total_nb_combinations << endl;

    //         Model &model_thread = models[thread_id];
            
    //         // get current combination
    //         int idx = count;
    //         vector<int> indices(nb_trains);
    //         for (int k = nb_trains - 1; k >= 0; k--)
    //         {
    //             indices[k] = idx % trips_combinations[k].size();
    //             idx /= trips_combinations[k].size();
    //         }
    //         vector<vector<int>> current;
    //         for (int k = 0; k < nb_trains; k++)
    //         {
    //             current.push_back(trips_combinations[k][indices[k]]);
    //         }

    //         // verify if combination is valid before calling for model to solve the combination
    //         if (is_valid_combination(data, current))
    //         {
    //             // model_thread.reset(data, nb_threads);
    //             model_thread.initialize(data, nb_threads);

    //             // call for model 
    //             bool reached_time_limit = false;
    //             int feasible = model_thread.run_with_routes_constraints(data, current, best_bound, time_limit_per_combination, reached_time_limit);
    //             if (reached_time_limit)
    //                 proved_optimal = false;
                
    //             // verify if feasible solution was found
    //             if (feasible)
    //             {
    //                 #pragma omp atomic
    //                 nb_feasible_combinations++;

    //                 #pragma omp critical
    //                 {
    //                     if (model_thread.best_sol[0].obj_value <= best_thread.best_sol[0].obj_value)
    //                     {
    //                         cout << "Found new best - " << counter_solved+1 << "/" << total_nb_combinations << endl;
    //                         best_thread = model_thread;
    //                         best_bound = model_thread.best_sol[0].obj_value;
    //                     }
    //                 }
    //             }
    //         }
    //         #pragma omp atomic
    //         counter_solved++;

    //         if (counter_solved % aux_progress == 0)
    //             cout << counter_solved/aux_progress * 10 << "%" << " done - " << counter_solved << "/" << total_nb_combinations << " combination(s) tested! (Thread " << thread_id << ")" << endl;

    //         // verify time limit
    //         std::chrono::time_point<std::chrono::steady_clock> now = chrono::steady_clock::now();
    //         chrono::duration<double> current_time = now - start;
    //         if (current_time.count() >= time_limit_complete)
    //         {
    //             #pragma omp critical
    //             {
    //                 cout << endl << ">> Reached time limit!" << endl;
    //                 cout << "\tTerminating process!" << endl;
    //                 if (nb_feasible_combinations == 0)
    //                 {
    //                     cout << "\tNo feasible solution was found..." << endl;
    //                 }
    //                 else
    //                 {
    //                     proved_optimal = false;
    //                     cout << "\tCannot prove optimality!" << endl;

    //                     best_thread.best_sol[0].computational_time = (current_time).count();
    //                     best_thread.get_solution(data, proved_optimal, "enum");
    //                 }
    //                 exit(0);
    //             }
    //         }
    //     }
    // }
}

/*------------------------------------------------------------------------*/
/*--------- Methods to generate and calculate trips combinations ---------*/
/*------------------------------------------------------------------------*/

void Combinations::calculate_trips_combinations(Data &data)
{
    trips_combinations.resize(nb_trains);
    for (int i = 0; i < nb_trains; i++)
    {
        // verify whether trips were already calculated for the train
        bool skip = false;
        for (int j = i-1; j >= 0; j--)
        {
            if (data.get_train_max_trips(j) == data.get_train_max_trips(i))
            {
                trips_combinations[i] = trips_combinations[j];
                skip = true;
                break;
            }
        }
        if (!skip)
        {
            if (verify_overflow(nb_routes, data.get_train_max_trips(i)))
            {
                cout << endl << ">> Too many combinations. Instance can't be solved by enumeration method!" << endl;
                return;
            }
            int nb_trips_comb_for_train = pow(nb_routes+1, data.get_train_max_trips(i)); // using nb_routes+1 to consider that no route is made
            
            // calculate number of trips combinations
            generate_trips_combinations(data, i);
        }
    }
}

void Combinations::generate_trips_combinations(Data &data, int train_idx)
{
    int nb_trips_comb_for_train = pow(nb_routes+1, data.get_train_max_trips(train_idx));

    vector<int> current(data.get_train_max_trips(train_idx), 0);
    for (unsigned long long count = 0; count < nb_trips_comb_for_train; count++)
    {
        // verify feasibility of the current combination before adding to the vector
        if (check_trips_feasibility(data, current))
        {
            trips_combinations[train_idx].push_back(current);
        }

        // go to next combination
        for (int i = data.get_train_max_trips(train_idx)-1; i >= 0; i--)
        {
            if (++current[i] < nb_routes+1)
                break;
            current[i] = 0;
        }
    }
}

bool Combinations::verify_overflow(unsigned long long base, unsigned long long exp)
{
    unsigned long long result = 1;
    for (unsigned long long i = 0; i < exp; i++)
    {
        if (result > ULLONG_MAX / base)
            return true;
        result *= base;
    }
    return false;
}

/*------------------------------------------------------------------------*/
/*--------- Methods to verify if combination is valid and should ---------*/
/*--------- be solved by the model.                              ---------*/
/*------------------------------------------------------------------------*/

bool Combinations::is_valid_combination (Data &data, vector<vector<int>> &current)
{
    if (!normalize_combination(data, current))
    {
        return false; // return false if combination was already previously executed
    }

    // verify whether demands were met
    vector <int> demands_per_day (data.get_nb_vertices(), 0);
    for (int i = 0; i < data.get_nb_vertices(); i++)
    {
        for (int j = 0; j < data.get_nb_intervals(); j++)
            demands_per_day[i] += data.get_demands()[i][j];
    }

    vector<int> times_vertex_was_visited (data.get_nb_vertices(), 0);
    for (int i = 0; i < current.size(); i++)
    {
        for (int j = 0; j < current[i].size(); j++)                        
        {
            if (current[i][j] != data.get_nb_routes() && current[i][j] != -1)
            {
                int route = current[i][j];
                for (auto vertex : data.get_route_vertices(route))
                { 
                    times_vertex_was_visited[vertex]++;
                }
            }
        }
    }
    for (int i = 0 ; i < data.get_nb_vertices(); i++)
    {
        if (times_vertex_was_visited[i] < data.get_demand_per_day()[i]) return false;
    }
    return true;
}


bool Combinations::check_trips_feasibility (Data &data, vector<int> &current)
{
    // verify whether first route starts at the initial depot
    if (current[0] != nb_routes)
    {
        if (!data.is_valid_route(0, 0, current[0])) return false;
    }

    bool flag = false; // flag to tell whether train completes any trips during the day
    for (int i = 0; i < current.size()-1; i++)
    {
        if (current[i] != nb_routes)
        {
            flag = true;
            if (current[i+1] != nb_routes)
            {
                // verify whether subsequential routes are compatible
                if (data.are_incompatible_routes(current[i], current[i+1])) return false;
            }
        }
        else if (current[i] == nb_routes)
        {
            // if trip is not made, verify whether the next ones also are not made
            if (i != current.size()-1 && current[i+1] != nb_routes) return false;
        }
    }

    if (current.front() != nb_routes) flag = true;

    return flag; 
}

/*-----------------------------------------------------------------*/
/* ====================== HEURISTIC METHODS ====================== */
/*-----------------------------------------------------------------*/

void Combinations::execute_heuristic (Data &data)
{
    best_thread.best_sol.push_back(best_thread.current_sol);

    heuristic.create_initial_candidates(data);
    total_nb_combinations = heuristic.candidate_combinations.size();

    int iter_set = 0;
    int iter = 0;
    bool tried_new = false;
    vector<vector<vector<int>>> current;
    while (true)
    {
        // verify time limit
        end = chrono::steady_clock::now();
        chrono::duration<double> current_time = end-start;
        if (current_time.count() >= time_limit_complete)
        {
            cout << ">> Reached time limit!" << endl;
            if (nb_feasible_combinations == 0)
            {
                cout << "\tNo feasible solution was found..." << endl;
                exit(0);
            }
            else
            {
                return;
            }
        }

        bool improved = false;

        if (heuristic.candidate_combinations.empty())
        {
            cout << "\t> Couldn't find any feasible combinations!" << endl;
            exit(0);
        }

        // get current set of combinations from heuristic
        // cout << "Iter " << iter+1 << ": " << endl;
        total_nb_combinations = heuristic.candidate_combinations.size();
        improved = execute_candidate_combinations(data);

        if (stop_execution.load()) return;

        if (nb_feasible_combinations == 0)
        {
            cout << "No feasible solution was found..." << endl;

            iter_set++;
            iter++;

            if (!heuristic.remove_trips(data, true, current[0], iter_set))
            {
                if (tried_new)
                {
                    // flexibilizar todos
                    heuristic.change_trips(data, true, current[0]);
                    continue;
                }

                // se entrar aqui mais de uma vez: ou rodar flexibilizar todas combinations (custoso)
                // ou incrementar novamente o conjunto de rotas
                cout << "Going to try a new set of routes..." << endl;
                heuristic.try_new_set_of_routes(data, iter_set);
                iter_set = 0;
                tried_new = true;
            }
            continue;
        }

        iter_set++;
        iter++;

        // if feasible solution was found...
        best_thread.get_best_combinations(data, current);

        if (improved)
        {
            if (current.size() > 1)
                best_thread.tie_breaker(data, current);
                
            if (!heuristic.remove_trips(data, false, current[0], iter)) // decrease number of trips till demands are not met
            {
                cout << "Terminating loop at iteration " << iter << endl;
                break;
            }    
        }
        else
        {
            break;
        }
    }

    // execute tie breaker
    if (current.size() > 1)
        best_thread.tie_breaker(data, current);
    
    // allow model to choose the last trips completed by the trains
    heuristic.change_trips(data, false, current[0]);
    best_thread.best_sol.push_back(best_thread.current_sol);
    best_thread.initialize(data, nb_threads);
    bool reached_time_limit = false;
    best_thread.run_with_routes_constraints(data, current[0], best_bound, time_limit_per_combination, reached_time_limit);

    best_thread.get_best_combinations(data, current);
}

bool Combinations::execute_candidate_combinations (Data &data)
{
    bool improved = false;

    cout << "Solving candidate combinations..." << endl << endl;;
    counter_solved = 0;

    // vectors with models for each thread
    vector<Model> models(nb_threads);

    #pragma omp parallel num_threads(nb_threads)
    {
        int thread_id = omp_get_thread_num();
        models[thread_id].best_sol.push_back(models[thread_id].current_sol);
        models[thread_id].initialize(data, nb_threads);

        #pragma omp for schedule(dynamic)
        // #pragma omp for
        for (unsigned long long count = 0; count < heuristic.candidate_combinations.size(); count++)
        {
            // if another thread told to stop execution
            if (stop_execution.load()) continue;

            Model &model_thread = models[thread_id];
            auto current = heuristic.candidate_combinations[count];

            if (normalize_combination(data, current))
            {
                model_thread.reset(data, nb_threads);
                bool reached_time_limit = false;
                int feasible = model_thread.run_with_routes_constraints(data, current, best_bound, time_limit_per_combination, reached_time_limit);

                if (feasible)
                {
                    #pragma omp atomic
                    nb_feasible_combinations++;

                    #pragma omp critical
                    {
                        if (model_thread.best_sol[0].obj_value < best_thread.best_sol[0].obj_value)
                        {
                            cout << "Found new best - ";
                            improved = true;
                            best_thread = model_thread;
                            best_bound = model_thread.best_sol[0].obj_value;
                        }
                        else if (model_thread.best_sol[0].obj_value == best_thread.best_sol[0].obj_value)
                        {
                            cout << "Found the same - ";
                            for (int i = 0; i < model_thread.best_sol.size(); i++)
                            {
                                best_thread.best_sol.push_back(model_thread.best_sol[i]);
                            }
                        }
                    }
                }
            }

            // #pragma omp atomic
            #pragma omp critical
            {
                counter_solved++;
                cout << counter_solved << "/" << heuristic.candidate_combinations.size() << " candidate combination(s) tested! (Thread " << thread_id << ")" << endl;
            }

            // verify time limit
            std::chrono::time_point<std::chrono::steady_clock> now = chrono::steady_clock::now();
            chrono::duration<double> current_time = now - start;
            if (current_time.count() >= time_limit_complete)
            {
                #pragma omp critical
                {
                    cout << endl << ">> Reached time limit!" << endl;
                    cout << "\tTerminating process!" << endl;
                    if (nb_feasible_combinations == 0)
                    {
                        cout << "\tNo feasible solution was found..." << endl;
                    }
                    else
                    {
                        best_thread.best_sol[0].computational_time = (current_time).count();
                        best_thread.get_solution(data, proved_optimal, "heuristic");
                    }
                    exit(0);
                }
            }
        }
    }

    return improved;
}

/*------------------------------------------------------------------------*/
/*=========================== GENERAL METHODS ============================*/
/*------------------------------------------------------------------------*/

bool Combinations::normalize_combination (Data &data, vector<vector<int>> &current)
{
    // normalize combinations to verify whether it was already tested previously
    // (only changes the train that will complete the trips)
    // return false if it was already tested and returns true if it's a new combination

    // make sure all dimensions have the same size
    for (int i = 0; i < current.size(); i++)
    {
        while (current[i].size() < data.get_max_nb_trips())
            current[i].push_back(nb_routes);
    }
    // normalize the vector
    vector<vector<int>> normalized_combination = current;
    sort(normalized_combination.begin(), normalized_combination.end());

    // verify whether it was already added according to the result
    pair<set<vector<vector<int>>>::iterator, bool> result;
    #pragma omp critical
    {
        result = unique_combinations.insert(normalized_combination);
    }
    if (!result.second)
    {
        return false; // combination already exists
    }
    return true; // current combination is a new combination
}





