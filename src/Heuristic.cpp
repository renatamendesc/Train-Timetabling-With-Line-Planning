#include <Heuristic.hpp>

using namespace std;

Heuristic::Heuristic(Data &data, int nb_threads, int time_limit_complete, int time_limit_per_combination)
{
    this->time_limit_complete = time_limit_complete;
    this->time_limit_per_combination = time_limit_per_combination;

    this->nb_threads = nb_threads;

    start = chrono::steady_clock::now();
    execute_heuristic(data);
    end = chrono::steady_clock::now();

    cout << endl << "-> Total time = " << chrono::duration<double>(end-start).count() << endl;
    if (!overall_best_sol.feasible)
    {
        cout << "No feasible solution was found with heuristic method!" << endl;
    }
    else
    {
        // display best solution
        overall_best_sol.display_solution(data);
    }
}

void Heuristic::execute_heuristic(Data &data)
{
    create_initial_candidates(data);
    cout << "Solving initial candidates..." << endl;
    solve_initial_candidates(data);
    while (improved_sol)
    {
        
        iter++;

        // verify time limit
        end = chrono::steady_clock::now();
        chrono::duration<double> current_time = end-start;
        if (current_time.count() >= time_limit_complete)
        { 
            reached_time_limit(data, current_time);
        }

        // create subsets of candidate combinations
        bool valid_subsets = create_subsets_from_best_combination(data);
        if (!valid_subsets)
            break;

        cout << "Updating candidates - iter = " << iter << endl;

        // update candidates with subsets
        execute_candidate_combinations(data);
    }
    cout << "Terminating loop at iteration " << iter << endl;

    // unfix final trips
    unfix_trips_from_best_combination(data);
    ModelORTools model;
    model.initialize(data, 1);
    model.create_model_with_routes_constraints(data, overall_best_sol.routes_combination);
    model.execute_solver_for_combination(data, overall_best_sol.obj_value, time_limit_per_combination, "heuristic");
}

void Heuristic::execute_candidate_combinations(Data &data)
{
    // vectors with model objects for each thread
    vector <ModelORTools> models(nb_threads);

    improved_sol = false;
    int counter_solved = 0;

    #pragma omp parallel num_threads(nb_threads)
    {
        int thread_id = omp_get_thread_num();
        #pragma omp for schedule(dynamic)
        for (unsigned long long count = 0; count < candidate_combinations.size(); count++)
        {
            // get current combination
            RoutesCombination current_combination = candidate_combinations[count];

            if (comb.normalize_combination(data, current_combination))
            {
                ModelORTools &model_thread = models[thread_id];
                model_thread.initialize(data, 1);
                model_thread.create_model_with_routes_constraints(data, current_combination);
                int feasible = model_thread.execute_solver_for_combination(data, overall_best_sol.obj_value, time_limit_per_combination, "heuristic");
                if (feasible)
                {
                    #pragma omp critical
                    {
                        if (model_thread.best_sol.obj_value < overall_best_sol.obj_value)
                        {
                            overall_best_sol = model_thread.best_sol;
                            improved_sol = true;
                        }
                        else if (model_thread.best_sol.obj_value == overall_best_sol.obj_value)
                        {
                            if (model_thread.best_sol.max_nb_repeated_routes > overall_best_sol.max_nb_repeated_routes)
                            {
                                overall_best_sol = model_thread.best_sol;
                                improved_sol = true;
                            }
                        }
                    }
                }
            }
            #pragma omp critical
            {
                counter_solved++;
                cout << counter_solved << "/" << candidate_combinations.size() << " candidate combination(s) tested! (Thread " << thread_id << ")" << endl;
            }

            // verify time limit
            chrono::time_point<std::chrono::steady_clock> now = chrono::steady_clock::now();
            chrono::duration<double> current_time = now-start;
            if (current_time.count() >= time_limit_complete)
            {
                #pragma omp critical
                {
                    reached_time_limit(data, current_time);
                }
            }
        }
    }
}

void Heuristic::solve_initial_candidates(Data &data)
{
    execute_candidate_combinations(data);
    if (improved_sol == false)
    {
        // create subsets from all candidates combinations
        if (create_subsets_from_all_candidates(data))
        {
            execute_candidate_combinations(data);
            if (improved_sol == false)
            {
                // update set of valid routes
                try_new_set_of_routes(data);
                execute_candidate_combinations(data);
                if (improved_sol == false)
                {
                    // allow model to choose the last trips completed by the trains
                    unfix_trips_from_best_combination(data);
                    execute_candidate_combinations(data);
                }
            }
        }
    }
}

void Heuristic::reached_time_limit(Data &data, std::chrono::duration<double> time)
{
    cout << endl << ">> Reached time limit!" << endl;
    cout << "\tTerminating process!" << endl;
    if (overall_best_sol.feasible == false)
    {
        cout << "\tNo feasible solution was found..." << endl;
    }
    cout << endl << "-> Total time = " << time.count() << endl;
    exit(0);
}

void Heuristic::create_initial_candidates (Data &data)
{
    create_cyclical_routes_set(data);
    create_initial_valid_routes_set(data);

    cout << endl << endl << "Creating initial set of candidate combinations..." << endl;

    // create maximum size combinations
    create_maximum_size_candidates(data);
}

void Heuristic::create_maximum_size_candidates(Data &data)
{
    int nb_trains = data.get_nb_trains();
    int num_valid = initial_valid_routes_set.size();
    int num_cyclic = cyclical_routes_set.size();

    // map initial routes done by each train
    vector<int> initial_indices(nb_trains, 0);
    bool done_initial = false;
    while (!done_initial)
    {
        // map cyclical routes done by each train
        vector<int> cyclic_indices(nb_trains, 0);
        bool done_cyclic = false;
        while (!done_cyclic)
        {
            vector<vector<int>> combination;
            bool valid_combination = true;
            for (int t = 0; t < nb_trains; t++)
            {
                int nb_trips = data.get_train_max_trips(t);
                vector<int> routes_of_train(nb_trips);

                // choose initial trip
                routes_of_train[0] = initial_valid_routes_set[initial_indices[t]];

                // choose cyclical route
                for (int k = 1; k < nb_trips; k++)
                {
                    routes_of_train[k] = cyclical_routes_set[cyclic_indices[t]];
                }

                // verify whether sequence of routes is compatible
                if (verify_compatibility(data, routes_of_train))
                {
                    combination.push_back(routes_of_train);
                }
                else
                {
                    valid_combination = false;
                }
            }

            // verify whether demands are met
            // pending: rever essa parte
            if (valid_combination && verify_demands(data, combination) && normalize_candidates(data, combination))
            {
                candidate_combinations.push_back(combination);
            }

            if (data.get_max_nb_trips() == 1)
                break;

            // increment cyclical indeces
            for (int i = nb_trains - 1; i >= 0; i--)
            {
                if (++cyclic_indices[i] < num_cyclic)
                {
                    break;
                }
                else
                {
                    cyclic_indices[i] = 0;
                    if (i == 0) done_cyclic = true;
                }
            }
        }

        // increment initial indices
        for (int i = nb_trains - 1; i >= 0; i--)
        {
            if (++initial_indices[i] < num_valid)
            {
                break;
            }
            else
            {
                initial_indices[i] = 0;
                if (i == 0) done_initial = true;
            }
        }
    }
}

// functions to create subsets from routes combinations

bool Heuristic::create_subsets_from_best_combination(Data &data)
{
    vector<RoutesCombination> new_candidates;
    remove_trips(data, new_candidates, overall_best_sol.routes_combination, data.get_max_nb_trips()-iter);

    if (new_candidates.empty())
    {
        return false;
    }
    else
    {   candidate_combinations = new_candidates;
        return true;
    }
}

bool Heuristic::create_subsets_from_all_candidates(Data &data)
{
    vector<vector<vector<int>>> new_candidates;
    for (int i = 0; i < candidate_combinations.size(); i++)
    {
        remove_trips(data, new_candidates, candidate_combinations[i], data.get_max_nb_trips()-iter);
    }

    if (new_candidates.empty())
    {
        return false;
    }
    else
    {   candidate_combinations = new_candidates;
        return true;
    }
}

void Heuristic::remove_trips (Data &data, vector<RoutesCombination> &new_candidates, RoutesCombination &current_combination, int max_nb_trips)
{
    size_t total_combinations = 1 << data.get_nb_trains();     // 2^n possibilities
    for (size_t mask = 0; mask < total_combinations; mask++)
    {
        if (mask == 0) continue;
        RoutesCombination new_combination;

        bool valid = true;
        for (size_t i = 0; i < data.get_nb_trains(); i++)
        {
            // if bit is activated, try to remove it
            if ((mask >> i) & 1)
            {
                // verify if bit is from a trip that can be removed
                if (current_combination[i].size() < max_nb_trips)
                {
                    valid = false;
                    break;
                }
                new_combination.push_back(vector<int>(current_combination[i].begin(), current_combination[i].end() - 1));
            }
            else
            {
                new_combination.push_back(current_combination[i]);         // trips stay the same for the train
            }
        }

        // pending: modificar funcao que verifica se a combinação é válida e se atende as demandas
        if (valid && verify_demands(data, new_combination) && normalize_candidates(data, new_combination))
        {
            new_candidates.push_back(new_combination);
        }
    }
}

void Heuristic::unfix_trips_from_best_combination (Data &data)
{
    // last route is free for the model to choose
    for (int i = 0; i < data.get_nb_trains(); i++)
    {
        if (overall_best_sol.routes_combination[i].empty())
        {
            overall_best_sol.routes_combination[i].push_back(-1);
            continue;
        }
        overall_best_sol.routes_combination[i].back() = -1;
    }
}

void Heuristic::unfix_trips_from_all_candidates (Data &data)
{
    candidate_combinations.clear();
    int count = 0;
    for (const auto& tested : unique_combinations)
    {
        count++;
        RoutesCombination candidate = tested;
        // last route is free for the model to choose
        for (int i = 0; i < data.get_nb_trains(); i++)
        {
            if (candidate[i].empty())
            {
                candidate[i].push_back(-1);
                continue;
            }
            candidate[i].back() = -1;
        }
        if (normalize_candidates(data, candidate))
            candidate_combinations.push_back(candidate);
    }
}

void Heuristic::display_candidates_combinations()
{
    for (int i = 0; i < candidate_combinations.size(); i++)
    {
        cout << "Combination " << i+1 << ": " << endl;
        for (int j = 0; j < candidate_combinations[i].size(); j++)
        {
            cout << "Train " << j << ": ";
            for (int k = 0; k < candidate_combinations[i][j].size(); k++)
            {
                cout << candidate_combinations[i][j][k] << " ";
            }
            cout << endl;
        }
        cout << endl << endl;
    }
}

// ==================================================================== //

bool Heuristic::verify_compatibility (Data &data, vector<int> &current)
{
    bool flag = false; // flag to tell whether train completes any trips during the day
    for (int i = 0; i < current.size()-1; i++)
    {
        if (current[i] != data.get_nb_routes())
        {
            flag = true;
            if (current[i+1] != data.get_nb_routes())
            {
                // verify whether subsequential routes are compatible
                if (data.are_incompatible_routes(current[i], current[i+1])) return false;
            }
        }
        else if (current[i] == data.get_nb_routes())
        {
            // if trip is not made, verify whether the next ones also are not made
            if (i != current.size()-1 && current[i+1] != data.get_nb_routes()) return false;
        }
    }

    if (current.front() != data.get_nb_routes()) flag = true;

    return flag; 
}

bool Heuristic::normalize_candidates (Data &data, vector<vector<int>> &current)
{
    // normalize combinations to verify whether it was already tested previously
    // (only changes the train that will complete the trips)
    // return false if it was already tested and returns true if it's a new combination

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

bool Heuristic::verify_demands(Data &data, vector<vector<int>> &current)
{
    // verify whether new subvector meets the demands
    int nb_trips_completed = 0;
    vector<int> times_vertex_was_visited (data.get_nb_vertices(), 0);
    for (int i = 0; i < current.size(); i++)
    {
        for (int j = 0; j < current[i].size(); j++)                        
        {
            nb_trips_completed++;
            int route = current[i][j];
            for (auto vertex : data.get_route_vertices(route))
            { 
                times_vertex_was_visited[vertex]++;
            }
        }
    }
    if (nb_trips_completed == 0) return false; 

    for (int i = 0 ; i < data.get_nb_vertices(); i++)
    {
        if (times_vertex_was_visited[i] < data.get_demand_per_day()[i]) return false;
    }
    return true;
}

void Heuristic::create_cyclical_routes_set(Data &data)
{
    cout << endl << "Selecting set of cyclical routes..." << endl;
    // // all cyclic routes included
    // for (int i = 0; i < data.get_nb_routes(); i++)
    // {
    //     if (data.is_cyclic_route(i))
    //         cyclical_routes_set.push_back(i);
    // }

    // search for full cycles included
    bool have_full_cycles = false;
    for (int i = 0; i < data.get_nb_routes(); i++)
    {
        if (data.get_route_vertices(i).size() == data.get_nb_vertices()+1)
        {
            cyclical_routes_set.push_back(i);
            have_full_cycles = true;
            have_cycles = true;
        }
    }

    if (!have_full_cycles)
    {
        for (int i = 0; i < data.get_nb_routes(); i++)
        {
            if ((data.is_cyclic_route(i)))
            {
                cyclical_routes_set.push_back(i);
                have_cycles = true;
            }
        }
    } 

    if (!have_cycles)
    {
        cout << "Instance does not have any cyclical routes..." << endl;
        exit(0);
    }
}

void Heuristic::create_initial_valid_routes_set(Data &data)
{
    cout << endl << "Selecting set of initial routes...";

    vector <int> will_be_removed (1, cyclical_routes_set.size()); // 0 for not being removed, 1 for being removed
    for (int i = 0; i < data.get_nb_routes(); i++)
    {
        if (data.is_valid_route(0, 0, i))
        {
            bool found_compatible = false;
            // verify whether initial route is compatible with any cyclical route before adding to set
            for (int j = 0; j < cyclical_routes_set.size(); j++)
            {
                if (!data.are_incompatible_routes(i, cyclical_routes_set[j]))
                {
                    found_compatible = true;
                    will_be_removed[j] = 0;
                }              
            }

            if (found_compatible) initial_valid_routes_set.push_back(i);
        }
    }

    // remove routes
    for (int i = 0; i < will_be_removed.size(); i ++)
    {
        if (will_be_removed[i] == 1)
        {
            cyclical_routes_set.erase(cyclical_routes_set.begin() + i);
        }
    }
}


void Heuristic::try_new_set_of_routes(Data &data)
{
    // add all cyclical routes
    cyclical_routes_set.clear();
    initial_valid_routes_set.clear();

    // all cyclic routes included
    for (int i = 0; i < data.get_nb_routes(); i++)
    {
        if (data.is_cyclic_route(i))
            cyclical_routes_set.push_back(i);
    }

    create_initial_valid_routes_set(data);
    create_maximum_size_candidates(data);
}