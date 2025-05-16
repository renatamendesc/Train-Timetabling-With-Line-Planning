#include <Combinations.hpp>

using namespace std;

Combinations::Combinations(Data &data)
{
    // reset directory that stores solutions for the instance
    reset_directory(data);

    // create object of the model
    Model model;
    model.initialize(data);

    // start counting time
    auto start = chrono::high_resolution_clock::now();

    // generate trips combinations for each train
    nb_routes = data.get_nb_routes();
    nb_trains = data.get_nb_trains();                                  
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
            int nb_trips_comb_for_train = pow(nb_routes, data.get_train_max_trips(i));        
            generate_trips_combinations(data, i); // calculate number of trips combinations
        }
    }

    // generate all combinations
    max_nb_trips = data.get_max_nb_trips();
    total_nb_combinations = 1;
    for (int i = 0; i < trips_combinations.size(); i++)
    {
        // calculate total number of possible combinations
        total_nb_combinations *= trips_combinations[i].size(); 
    }
    generate_all_combinations(data, model);

    // cout << endl;
    // for (int i = 0; i < all_combinations.size(); i++)
    // {
    //     cout << "Combination " << i << ": " << endl;
    //     for (int j = 0; j < all_combinations[i].size(); j++)
    //     {
    //         cout << "Train " << j << ": ";
    //         int idx_trip_comb = all_combinations[i][j];
    //         for (int k = 0; k < trips_combinations[idx_trip_comb].size(); k++)
    //         {
    //             cout << trips_combinations[idx_trip_comb][k] << " ";
    //         }
    //         cout << endl;
    //     }
    // }

    // finish counting time
    auto end = chrono::high_resolution_clock::now();
    std::chrono::duration<double> time = end-start;
    model.best_sol.computational_time = (time).count();

    // get optimal solution
    model.get_solution(data, true);
    cout << endl << nb_feasible_combinations << "/" << total_nb_combinations << " were feasible combination(s) (" << std::fixed << std::setprecision(5) << (double(nb_feasible_combinations) / total_nb_combinations) * 100 << "%)" << endl;
}

void Combinations::reset_directory(Data &data)
{
    // reseting past feasible solutions files for the instance
    std::string full_path =  "combinations/feasible-combinations/" + data.get_instance_name();
    if (std::filesystem::exists(full_path)) {
        for (const auto& entry : std::filesystem::directory_iterator(full_path)) {
            std::filesystem::remove_all(entry.path());
        }
    }
}

void Combinations::generate_all_combinations(Data &data, Model &model)
{
    for (unsigned long long count = 0; count < total_nb_combinations; count++)
    {
        // get current combination
        int idx = count;
        vector<int> indices(nb_trains);
        for (int k = nb_trains - 1; k >= 0; k--) {
            indices[k] = idx % trips_combinations[k].size();
            idx /= trips_combinations[k].size();
        }
        vector<vector<int>> current;
        for (int k = 0; k < nb_trains; k++) {
            current.push_back(trips_combinations[k][indices[k]]);
        }

        // check if its feasible
        if (check_final_feasibility(data, current))
        {
            // all_combinations.push_back(current);

            model.reset(data);
            bool feasible = model.run_with_routes_constraints(data, current);

            if (feasible)
            {
                nb_feasible_combinations++;
            }
        }
        cout << count+1 << "/" << total_nb_combinations << " combination(s) tested!" << endl;
    }
}

bool Combinations::check_final_feasibility (Data &data, vector<vector<int>> &current)
{

    // cout << current.size() << endl;
    // for (int i = 0; i < current.size(); i++)
    // {
    //     cout << "Trem " << i+1 << " - ";
    //     for (int j = 0; j < current[i].size(); j++)
    //     {
    //         cout << current[i][j] << " ";
    //     }
    //     cout << endl;
    // }

    // normalize combinations to verify whether it was already added
    // (only changes the train that will complete the trips)

    // make sure all dimensions have the same size
    for (int i = 0; i < current.size(); i++)
    {
        while (current[i].size() < max_nb_trips)
            current[i].push_back(nb_routes);
    }
    // normalize the vector
    vector<vector<int>> normalized_combination = current;
    sort(normalized_combination.begin(), normalized_combination.end());
    auto result = unique_combinations.insert(normalized_combination);
    if (!result.second)
    {
        return false; // combination already exists
    }

    // verify whether demands were met
    vector <int> demands_per_day (data.get_nb_vertices(), 0);
    for (int i = 0; i < data.get_nb_vertices(); i++)
    {
        for (int j = 0; j < data.get_nb_intervals(); j++)
            demands_per_day[i] += data.get_demands()[i][j];
    }
    vector<int> times_vertex_was_visited (data.get_nb_vertices(), 0);
    for (int i = 0; i < current.size(); i++)                                // for each train
    {
        for (int j = 0; j < current[i].size(); j++)                         // for each route completed by the train
        {
            if (current[i][j] != data.get_nb_routes())  // if trip is made
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
        if (times_vertex_was_visited[i] < demands_per_day[i]) return false;
    }

    return true;
}

void Combinations::generate_trips_combinations(Data &data, int train_idx)
{
    int nb_trips_comb_for_train = pow(nb_routes, data.get_train_max_trips(train_idx));

    vector<int> current(data.get_train_max_trips(train_idx), 0);
    for (unsigned long long count = 0; count < nb_trips_comb_for_train; count++)
    {
        // cout << "Combinação: ";
        // for (int i = 0; i < current.size(); i++)
        // {
        //     cout << current[i] << " ";
        // }
        // cout << endl;

        // verify feasibility of the current combination before adding to the vector
        if (check_trips_feasibility(data, current))
        {
            trips_combinations[train_idx].push_back(current);
        }

        // go to next combination
        for (int i = data.get_train_max_trips(train_idx)-1; i >= 0; i--)
        {
            if (++current[i] < nb_routes)
                break;
            current[i] = 0;
        }
    }
}

bool Combinations::check_trips_feasibility (Data &data, vector <int> &current)
{

    // verify whether first route starts at the initial depot
    if (!data.is_valid_route(0, 0, current[0])) return false;

    // verify whether subsequential routes are compatible
    for (int i = 0; i < current.size()-1; i++)
    {
        if (data.are_incompatible_routes(current[i], current[i+1])) return false;
    }

    return true; 
}






