#include <Combinations.hpp>

using namespace std;

Combinations::Combinations(Data &data, int threads, int strategy)
{
    // reset directory that stores solutions for the instance
    reset_directory(data);

    // start counting time
    auto start = chrono::steady_clock::now();

    nb_threads = threads;
    nb_routes = data.get_nb_routes();
    nb_trains = data.get_nb_trains();

    // if strategy: 0 -> enumeration, 1 -> heuristic
    if (strategy == 0) // executing enumeration
    {
        execute_enumeration(data);
        cout << "All combination(s) tested!" << endl;
    }
    else if (strategy == 1) // executing heuristic
    {
        execute_heuristic(data);
    }  

    // finish counting time
    auto end = chrono::steady_clock::now();
    chrono::duration<double> time = end-start;
    best_thread.best_sol.computational_time = (time).count();

    // get the time that it took to find the optimal solution
    double time_till_optimal = (chrono::duration<double>(best_thread.best_sol.time_found-start)).count();
    cout << fixed << setprecision(2) << "    -> Optimal was found = " << time_till_optimal << endl;

    // get optimal solution
    best_thread.get_solution(data, true);
    if (strategy == 0)
        cout << endl << nb_feasible_combinations << "/" << total_nb_combinations << " were feasible combination(s) (" << fixed << setprecision(5) << (double(nb_feasible_combinations) / total_nb_combinations) * 100 << "%)" << endl;
}

void Combinations::execute_enumeration(Data &data)
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
                cout << endl << ">> Too many combinations. Instance can't be solved!" << endl;
                return;
            }
            int nb_trips_comb_for_train = pow(nb_routes+1, data.get_train_max_trips(i)); // using nb_routes+1 to consider that no route is made
            
            // calculate number of trips combinations
            generate_trips_combinations(data, i);
        }
    }

    // generate all combinations
    total_nb_combinations = 1;
    for (int i = 0; i < trips_combinations.size(); i++)
    {
        // calculate total number of possible combinations
        total_nb_combinations *= trips_combinations[i].size(); 
    }

    cout << "Starting to test combinations... - Total number of combinations = " << total_nb_combinations << endl;
    execute_all_combinations(data);
}

void Combinations::execute_heuristic (Data &data)
{
    heuristic.create_initial_combinations(data);
    total_nb_combinations = heuristic.candidate_combinations.size();

    int iter = 0;
    bool not_done = true;
    vector<vector<int>> current;
    while (not_done)
    {
        // get current set of combinations from heuristic
        total_nb_combinations = heuristic.candidate_combinations.size();
        execute_candidate_combinations(data);

        if (nb_feasible_combinations == 0)
        {
            cout << "No feasible solution was found..." << endl;
            iter++;
            heuristic.create_subsets(data, iter);
            continue;
        }

        // if feasible solution was found...
        best_thread.get_combination(data, current);
        not_done = heuristic.remove_trips(data, true, current, data.get_max_nb_trips()-iter-1); // reduz até as demandas não serem cumpridas

        iter++;
    }

    heuristic.change_trips(data, current); // verificar possibilidade de ainda deixar outra rota livre
    if (check_final_feasibility(data, current))
    {
        best_thread.reset(data);
        best_thread.run_LP_with_routes_constraints(data, current);
    }
}

void Combinations::execute_all_combinations(Data &data)
{
    // variable to assist in displaying progress
    aux_progress = ceil(0.1 * total_nb_combinations);
    if (aux_progress == 0)
        aux_progress = 1;

    // vectors with models for each thread
    vector<Model> models(nb_threads);

    #pragma omp parallel num_threads(nb_threads)
    {
        int thread_id = omp_get_thread_num();
        models[thread_id].initialize(data);

        #pragma omp for
        for (unsigned long long count = 0; count < total_nb_combinations; count++)
        {
            Model &model_thread = models[thread_id];
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

            if (check_final_feasibility(data, current))
            {
                model_thread.reset(data);
                bool feasible = model_thread.run_LP_with_routes_constraints(data, current);

                if (feasible)
                {
                    #pragma omp atomic
                    nb_feasible_combinations++;

                    #pragma omp critical
                    {
                        if (model_thread.best_sol.obj_value <= best_thread.best_sol.obj_value)
                        {
                            best_thread = model_thread;
                        }
                    }
                }
            }

            #pragma omp atomic
            counter_solved++;

            if (counter_solved % aux_progress == 0)
                cout << counter_solved/aux_progress * 10 << "%" << " done - " << counter_solved << "/" << total_nb_combinations << " combination(s) tested! (Thread " << thread_id << ")" << endl;
        }
    }
}

void Combinations::execute_candidate_combinations (Data &data)
{
    // vectors with models for each thread
    vector<Model> models(nb_threads);

    #pragma omp parallel num_threads(nb_threads)
    {
        int thread_id = omp_get_thread_num();
        models[thread_id].initialize(data);

        #pragma omp for
        for (unsigned long long count = 0; count < heuristic.candidate_combinations.size(); count++)
        {
            Model &model_thread = models[thread_id];
            auto current = heuristic.candidate_combinations[count];

            cout << count << "/" << heuristic.candidate_combinations.size() << " - thread " << thread_id << endl;

            if (check_final_feasibility(data, current))
            {
                model_thread.reset(data);
                bool feasible = model_thread.run_LP_with_routes_constraints(data, current);

                if (feasible)
                {
                    #pragma omp atomic
                    nb_feasible_combinations++;

                    #pragma omp critical
                    {
                        if (model_thread.best_sol.obj_value <= best_thread.best_sol.obj_value)
                        {
                            best_thread = model_thread;
                        }
                    }
                }
            }

            #pragma omp atomic
            counter_solved++;
        }
    }
}

bool Combinations::check_final_feasibility (Data &data, vector<vector<int>> &current)
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
            if (current[i][j] != data.get_nb_routes())
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

bool Combinations::check_trips_feasibility (Data &data, vector<int> &current)
{
    // verify whether first route starts at the initial depot
    // if (!data.is_valid_route(0, 0, current[0])) return false;
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

void Combinations::reset_directory(Data &data)
{
    // reseting past feasible solutions files for the instance
    string full_path =  "combinations/feasible-combinations/" + data.get_instance_name();
    if (filesystem::exists(full_path))
    {
        for (const auto& entry : filesystem::directory_iterator(full_path))
        {
            filesystem::remove_all(entry.path());
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







