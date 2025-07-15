#include <Heuristic.hpp>

using namespace std;

void Heuristic::create_initial_candidates (Data &data)
{
    cout << endl << "Creating initial set of candidate combinations..." << endl;

    // próximo passo: criar conjunto a partir da mínima quantidade de viagens que atende as demandas

    create_cyclical_routes_set(data);
    create_initial_valid_routes_set(data);

    // adicionar viagens até cumprir as demandas

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
            if (valid_combination && verify_demands(data, combination))
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

    // for (int i = 0; i < candidate_combinations.size(); i++)
    // {
    //     cout << "Combinação " << i+1 << ": " << endl;
    //     for (int j = 0; j < candidate_combinations[i].size(); j++)
    //     {
    //         cout << "Trem " << j << ": ";
    //         for (int k = 0; k < candidate_combinations[i][j].size(); k++)
    //         {
    //             cout << candidate_combinations[i][j][k] << " ";
    //         }
    //         cout << endl;
    //     }
    //     cout << endl;
    // }
}

void Heuristic::create_subsets (Data &data, int iter)
{
    cout << endl << "Creating subsets from initial candidate combinations..." << endl;

    candidate_combinations.clear();

    int nb_trains = data.get_nb_trains();
    int num_valid = initial_valid_routes_set.size();
    int num_cyclic = cyclical_routes_set.size();

    vector <int> max_trips_iter (nb_trains);
    int max_nb_trips_iter = 0;
    for (int i = 0; i < nb_trains; i++)
    {
        if (data.get_train_max_trips(i) < data.get_max_nb_trips()-iter)
            max_trips_iter[i] = data.get_train_max_trips(i);
        else
            max_trips_iter[i] = data.get_max_nb_trips()-iter;

        if (max_nb_trips_iter < max_trips_iter[i])
            max_nb_trips_iter = max_trips_iter[i];
    }

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
            for (int t = 0; t < nb_trains; t++)
            {
                int nb_trips = max_trips_iter[t];
                vector<int> routes_of_train(nb_trips);

                // choose initial trip
                routes_of_train[0] = initial_valid_routes_set[initial_indices[t]];

                // choose cyclical route
                for (int k = 1; k < nb_trips; k++)
                {
                    routes_of_train[k] = cyclical_routes_set[cyclic_indices[t]];
                }
                combination.push_back(routes_of_train);
            }
            candidate_combinations.push_back(combination);

            if (max_nb_trips_iter == 1)
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
        for (int i = nb_trains-1; i >= 0; i--)
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

    // for (int i = 0; i < candidate_combinations.size(); i++)
    // {
    //     cout << "Combinação " << i+1 << ": " << endl;
    //     for (int j = 0; j < candidate_combinations[i].size(); j++)
    //     {
    //         cout << "Trem " << j << ": ";
    //         for (int k = 0; k < candidate_combinations[i][j].size(); k++)
    //         {
    //             cout << candidate_combinations[i][j][k] << " ";
    //         }
    //         cout << endl;
    //     }
    //     cout << endl;
    // }
}

bool Heuristic::remove_trips (Data &data, bool feasible, vector<vector<int>> &current, int min_nb_trips)
{
    cout << endl << "Removing trips from current best combination..." << endl;

    bool trips_met_demand = false;

    candidate_combinations.clear();
    vector<vector<vector<int>>> resultado;

    size_t total_combinations = 1 << data.get_nb_trains(); // 2^n possibilities
    for (size_t mask = 0; mask < total_combinations; mask++)
    {
        if (mask == 0) continue;
        vector<vector<int>> aux_combination;

        bool valid = true;
        for (size_t i = 0; i < data.get_nb_trains(); i++)
        {
            // if bit is activated, try to remove it
            if ((mask >> i) & 1)
            {
                // verify if bit is from a trip that can be removed
                if (current[i].size() < min_nb_trips)
                {
                    valid = false;
                    break;
                }
                aux_combination.push_back(vector<int>(current[i].begin(), current[i].end() - 1));
            }
            else
            {
                aux_combination.push_back(current[i]);  // trips stay the same for the train
            }
        }

        if (valid)
        {
            if (verify_demands(data, aux_combination))
            {
                candidate_combinations.push_back(aux_combination);
                trips_met_demand = true;  
            }
            else
            {
                trips_met_demand = false;
            }
        }
    }

    // cout << "Flexibilizando viagens..." << endl << endl;
    // for (int i = 0; i < candidate_combinations.size(); i++)
    // {
    //     cout << "Combinação " << i+1 << ": " << endl;
    //     for (int j = 0; j < candidate_combinations[i].size(); j++)
    //     {
    //         cout << "Trem " << j << ": ";
    //         for (int k = 0; k < candidate_combinations[i][j].size(); k++)
    //         {
    //             cout << candidate_combinations[i][j][k] << " ";
    //         }
    //         cout << endl;
    //     }
    //     cout << endl;
    // }

    return trips_met_demand;
}

void Heuristic::change_trips (Data &data, vector<vector<int>> &current)
{
    // final trip is chosen by the model
    // obs.: -1 = free, nb_routes = trip not made

    cout << endl << "Relaxing trips from current best solution, so the model can make changes..." << endl;

    // last route is free for the model to choose
    for (int i = 0; i < data.get_nb_trains(); i++)
    {
        current[i].back() = -1;
    }
    // first route is free for the model to choose
    // for (int i = 0; i < data.get_nb_trains(); i++)
    // {
    //     current[i].front() = -1;
    // }

    // cout << endl;
    // for (int i = 0; i < current.size(); i++)
    // {
    //     cout << "Trem " << i+1 << ": ";
    //     for (int j = 0; j < current[i].size(); j++)
    //     {
    //         cout << current[i][j] << " ";
    //     }
    //     cout << endl;
    // }

}

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

bool Heuristic::verify_demands(Data &data, vector<vector<int>> &current)
{
    // cout << endl;
    // for (int i = 0; i < current.size(); i++)
    // {
    //     cout << "trem " << i << ": ";
    //     for (int j = 0; j < current[i].size(); j++)                        
    //     {
    //         cout << current[i][j] << " ";
    //     }
    //     cout << endl;
    // }

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
    // // all cyclic routes included
    // for (int i = 0; i < data.get_nb_routes(); i++)
    // {
    //     if (data.is_cyclic_route(i))
    //         cyclical_routes_set.push_back(i);
    // }

    // only full cycles included
    for (int i = 0; i < data.get_nb_routes(); i++)
    {
        if ((data.is_cyclic_route(i)) && (data.get_route_vertices(i).size() == data.get_nb_vertices()+1))
            cyclical_routes_set.push_back(i);
    }

    // cout << "Rotas cíclicas válidas: " << endl;
    // for (int i  = 0; i < cyclical_routes_set.size(); i++)
    // {
    //     cout << cyclical_routes_set[i] << " ";
    // }
    // cout << endl << endl;
}

void Heuristic::create_initial_valid_routes_set(Data &data)
{
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

    // cout << "------------------------" << endl;
    // cout << "Rotas iniciais válidas: " << endl;
    // for (int i  = 0; i < initial_valid_routes_set.size(); i++)
    // {
    //     cout << initial_valid_routes_set[i] << " ";
    // }
    // cout << endl << endl;

    // cout << "Rotas cíclicas válidas (após remoção): " << endl;
    // for (int i  = 0; i < cyclical_routes_set.size(); i++)
    // {
    //     cout << cyclical_routes_set[i] << " ";
    // }
    // cout << endl << endl;
}