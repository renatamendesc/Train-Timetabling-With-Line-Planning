#include <Combinations.hpp>
#include "CombinationsKernel.cuh"
#include <cuda_runtime.h>

using namespace std;

// --------------------------------------------------
// 3) Host: cópia do demand_per_day e lançamento
// --------------------------------------------------
void Combinations::execute_all_combinations_hybrid(Data &data) {
    // 1) compute total combinations and trips_sizes
    total_nb_combinations = 1;
    std::vector<int> trips_sizes(nb_trains);
    for (int k = 0; k < nb_trains; ++k) {
        trips_sizes[k] = trips_combinations[k].size();
        total_nb_combinations *= trips_sizes[k];
    }
    aux_progress = std::ceil(0.1 * total_nb_combinations);
    if (aux_progress == 0) aux_progress = 1;

    // 2) prepare models per CPU thread
    std::vector<Model> models(nb_threads);
    for (int t = 0; t < nb_threads; ++t)
        models[t].initialize(data);

    cudaMalloc(&d_trips_sizes,       nb_trains*sizeof(int));
    cudaMalloc(&d_trip_route_idx,    nb_trains*max_routes_per_train*sizeof(int));
    cudaMalloc(&d_route_lengths,     total_routes*sizeof(int));
    cudaMalloc(&d_route_offsets,     total_routes*sizeof(int));
    cudaMalloc(&d_flat_routes,       total_flat*sizeof(int));
    cudaMalloc(&d_demand_per_day,    nb_vertices*sizeof(int));

    // Copy constant GPU data once (outside OMP region)
    cudaMemcpy(d_trips_sizes,       trips_sizes.data(),
               nb_trains * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_trip_route_idx,    host_trip_route_idx,
               nb_trains * max_routes_per_train * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_route_lengths,     host_route_lengths,
               total_routes * sizeof(int),  cudaMemcpyHostToDevice);
    cudaMemcpy(d_route_offsets,     host_route_offsets,
               total_routes * sizeof(int),  cudaMemcpyHostToDevice);
    cudaMemcpy(d_flat_routes,       host_flat_routes,
               total_flat * sizeof(int),   cudaMemcpyHostToDevice);

    int nb_vertices = data.get_nb_vertices();
    cudaMemcpy(d_demand_per_day,
               data.get_demand_per_day().data(),
               nb_vertices * sizeof(int),
               cudaMemcpyHostToDevice);

    // 3) parallel region over CPU threads
    #pragma omp parallel num_threads(nb_threads)
    {
        int tid = omp_get_thread_num();
        cudaStream_t stream;
        cudaStreamCreate(&stream);

        unsigned long long chunk_size =
            (total_nb_combinations + nb_threads - 1) / nb_threads;
        unsigned long long start_idx = tid * chunk_size;
        unsigned long long end_idx   = std::min(start_idx + chunk_size, total_nb_combinations);
        size_t n_chunk = end_idx - start_idx;

        bool* d_flags = nullptr;
        cudaMallocAsync(&d_flags, n_chunk * sizeof(bool), stream);

        int threadsPerBlock = 256;
        int blocks = (n_chunk + threadsPerBlock - 1) / threadsPerBlock;
        size_t shared_mem = nb_trains * sizeof(int);
         processCombinationsKernel<<<
            blocks, threadsPerBlock, shared_mem, stream
        >>>(
            d_trips_sizes,
            d_trip_route_idx,
            d_route_lengths,
            d_route_offsets,
            d_flat_routes,
            d_demand_per_day,      
            nb_vertices,           
            nb_trains,
            max_routes_per_train,
            total_nb_combinations,
            start_idx,
            d_flags
        );
        cudaStreamSynchronize(stream);

        std::vector<char> h_flags(n_chunk);
        cudaMemcpyAsync(&h_flags[0], d_flags,
                        n_chunk * sizeof(bool),
                        cudaMemcpyDeviceToHost,
                        stream);
        cudaStreamSynchronize(stream);

        Model &model = models[tid];
        for (size_t offset = 0; offset < n_chunk; ++offset) {
            if (!h_flags[offset]) continue;
            unsigned long long count = start_idx + offset;
            unsigned long long idx = count;
            std::vector<int> indices(nb_trains);
            for (int k = nb_trains - 1; k >= 0; --k) {
                indices[k] = idx % trips_combinations[k].size();
                idx /= trips_combinations[k].size();
            }
            std::vector<std::vector<int>> current(nb_trains);
            for (int k = 0; k < nb_trains; ++k)
                current[k] = trips_combinations[k][indices[k]];

            // full feasibility check on CPU
            if (!check_final_feasibility(data, current)) continue;

            model.reset(data);
            bool feas = model.run_with_routes_constraints(data, current, best_bound);
            if (feas) {
                #pragma omp atomic
                nb_feasible_combinations++;
                #pragma omp critical
                {
                    if (model.best_sol.obj_value <= best_thread.best_sol.obj_value) {
                        best_thread = model;
                        best_bound  = model.best_sol.obj_value;
                    }
                }
            }
        }

        #pragma omp atomic
        counter_solved += n_chunk;
        if (counter_solved % aux_progress == 0) {
            std::cout << (counter_solved/aux_progress) * 10 << "% done - "
                      << counter_solved << "/" << total_nb_combinations
                      << " (Thread " << tid << ")" << std::endl;
        }

        cudaFreeAsync(d_flags, stream);
        cudaStreamDestroy(stream);
    }
}



Combinations::Combinations(Data &data, int threads, int strategy)
{
    // start counting time
    auto start = chrono::steady_clock::now();

    nb_threads = threads;
    nb_routes = data.get_nb_routes();
    nb_trains = data.get_nb_trains();

    // if strategy == 0 -> enumeration, if == 1 -> heuristic
    if (strategy == 0)      // executing enumeration
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

    // // get the time that it took to find the optimal solution
    // double time_till_optimal = (chrono::duration<double>(best_thread.best_sol.time_found-start)).count();
    // cout << fixed << setprecision(2) << "    -> Optimal was found = " << time_till_optimal << endl;

    // get optimal solution
    best_thread.get_solution(data, true);

    // get number of feasible solutions
    // if (strategy == 0)
    //     cout << endl << nb_feasible_combinations << "/" << total_nb_combinations << " were feasible combination(s) (" << fixed << setprecision(5) << (double(nb_feasible_combinations) / total_nb_combinations) * 100 << "%)" << endl;
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
                cout << endl << ">> Too many combinations. Instance can't be solved by enumeration method!" << endl;
                return;
            }
            int nb_trips_comb_for_train = pow(nb_routes+1, data.get_train_max_trips(i)); // using nb_routes+1 to consider that no route is made
            
            // calculate number of trips combinations
            generate_trips_combinations(data, i);
        }
    }

    // calculate total number of possible combinations
    total_nb_combinations = 1;
    for (int i = 0; i < trips_combinations.size(); i++)
    {
        total_nb_combinations *= trips_combinations[i].size(); 
    }

    cout << "Starting to test combinations... - Total number of combinations = " << total_nb_combinations << endl;
    execute_all_combinations_hybrid(data);
}

void Combinations::execute_heuristic (Data &data)
{
    heuristic.create_initial_candidates(data);
    total_nb_combinations = heuristic.candidate_combinations.size();

    int iter = 0;
    bool not_done = true;
    vector<vector<int>> current;
    while (not_done)
    {
        // get current set of combinations from heuristic
        // cout << "Iter " << iter+1 << ": " << endl;
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
        not_done = heuristic.remove_trips(data, true, current, data.get_max_nb_trips()-iter-1); // decrease number of trips till demands are not met

        iter++;
    }

    heuristic.change_trips(data, current);
    best_thread.initialize(data);
    best_thread.run_with_routes_constraints(data, current, best_bound);
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

        #pragma omp for schedule(dynamic)
        // #pragma omp for
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
                bool feasible = model_thread.run_with_routes_constraints(data, current, best_bound);

                if (feasible)
                {
                    #pragma omp atomic
                    nb_feasible_combinations++;

                    #pragma omp critical
                    {
                        if (model_thread.best_sol.obj_value <= best_thread.best_sol.obj_value)
                        {
                            best_thread = model_thread;
                            best_bound = model_thread.best_sol.obj_value;
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
    counter_solved = 0;

    // vectors with models for each thread
    vector<Model> models(nb_threads);

    #pragma omp parallel num_threads(nb_threads)
    {
        int thread_id = omp_get_thread_num();
        models[thread_id].initialize(data);

        #pragma omp for schedule(dynamic)
        // #pragma omp for
        for (unsigned long long count = 0; count < heuristic.candidate_combinations.size(); count++)
        {
            Model &model_thread = models[thread_id];
            auto current = heuristic.candidate_combinations[count];

            if (normalize_combination(data, current))
            {
                model_thread.reset(data);
                bool feasible = model_thread.run_with_routes_constraints(data, current, best_bound);

                if (feasible)
                {
                    #pragma omp atomic
                    nb_feasible_combinations++;

                    #pragma omp critical
                    {
                        if (model_thread.best_sol.obj_value <= best_thread.best_sol.obj_value)
                        {
                            best_thread = model_thread;
                            best_bound = model_thread.best_sol.obj_value;
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







