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

    // generate trips combinations
    nb_routes = data.get_nb_routes();                                  // calculate number of trips combinations           
    max_nb_trips = data.get_max_nb_trips();
    max_nb_trips_combinations = pow(nb_routes+1, max_nb_trips);        // sums 1 to the routes to consider the possibility that the trip might not be made
    generate_trips_combinations(data);

    // generate all combinatinos
    nb_trains = data.get_nb_trains();                                  // calculate total number of possible combinations
    total_nb_combinations = pow(trips_combinations.size(), nb_trains);
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
    vector<int> current(nb_trains, 0);
    for (unsigned long long count = 0; count < total_nb_combinations; count++)
    {
        if (check_final_feasibility(data, current))
        {
            cout << "Válida" << endl << endl;
            all_combinations.push_back(current);

            // create vector that stores explicitly the current combination
            vector<vector<int>> routes_of_trains;
            for (int i = 0; i < current.size(); i++)
            {
                routes_of_trains.push_back(trips_combinations[current[i]]);
            }

            // reset the model and executes it with routes constraints
            model.reset(data);
            bool feasible = model.run_with_routes_constraints(data, routes_of_trains);
            if (feasible)
            {
                nb_feasible_combinations++;
            }
        }
        cout << count << "/" << total_nb_combinations << " combination(s) tested!" << endl;

        // go to next combination
        for (int i = nb_trains-1; i >= 0; i--)
        {
            if (++current[i] < trips_combinations.size())
                break;
            current[i] = 0;
        }
    }
}

bool Combinations::check_final_feasibility (Data &data, vector <int> &current)
{
    for (int i = 0; i < current.size(); i++)
    {
        cout << "Train " << i << ": ";
        for (int j = 0; j < trips_combinations[current[i]].size(); j++)
        {
            cout << trips_combinations[current[i]][j] << " ";
        }
        cout << endl;
    }
    cout << endl;

    // verify whether number of trips is feasible
    for (int i = 0; i < current.size(); i++)
    {
        int trips_completed = 0;
        for (int j = 0; j < trips_combinations[current[i]].size(); j++)
        {
            if (trips_combinations[current[i]][j] != nb_routes)
                trips_completed++;
        }
        if (trips_completed > data.get_train_max_trips(i))
            return false;
    }

    // normalize combinations to verify whether it was already added
    // (only changes the train that will complete the trips)
    vector<int> normalized_combination = current;
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
        for (int j = 0; j < trips_combinations[current[i]].size(); j++)     // for each route completed by the train
        {
            if (trips_combinations[current[i]][j] != data.get_nb_routes())  // if trip is made
            {
                int route = trips_combinations[current[i]][j];
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

void Combinations::generate_trips_combinations(Data &data)
{
    vector<int> current(max_nb_trips, 0);
    for (unsigned long long count = 0; count < max_nb_trips_combinations; count++)
    {
        // verify feasibility of the current combination before adding to the vector
        if (check_trips_feasibility(data, current) && prohibited_combinations.find(current) == prohibited_combinations.end())
        {
            trips_combinations.push_back(current);
        }
        else
        {
            add_to_prohibited_set(current);
        }

        // go to next combination
        for (int i = max_nb_trips-1; i >= 0; i--)
        {
            if (++current[i] < nb_routes+1)
                break;
            current[i] = 0;
        }
    }
}

bool Combinations::check_trips_feasibility (Data &data, vector <int> &current)
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

void Combinations::add_to_prohibited_set(vector <int> invalid_combination)
{
    prohibited_combinations.insert(invalid_combination);
    for (int i = invalid_combination.size()-1; i >= 0; i--)
    {
        invalid_combination[i] == nb_routes;
        prohibited_combinations.insert(invalid_combination);
    }
}







