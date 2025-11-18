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
}

Heuristic::execute_heuristic(Data &data)
{
    create_initial_candidates(data);
    // solve initial

    while (current_best_sol.feasible && current_best_sol.obj_value < overall_best_sol.obj_value)
    {
        overall_best_sol = current_best_sol;

        // create subsets of candidate combinations
        // verify if subsets meet demands

        // update candidates with subsets
    }
}

// ================================================================ //

void Heuristic::create_initial_candidates (Data &data)
{
    // próximo passo: criar conjunto a partir da mínima quantidade de viagens que atende as demandas
    create_cyclical_routes_set(data);
    create_initial_valid_routes_set(data);

    cout << endl << endl << "Creating initial set of candidate combinations..." << endl;

    // adicionar viagens até cumprir as demandas
    // add_trips_till_demands_are_met(data, nb_trains, num_valid, num_cyclic);

    // create maximum size combinations
    create_maximum_size_candidates(data);
}

void Heuristic::add_trips_till_demands_are_met(Data &data, int nb_trains, int num_valid, int num_cyclic)
{
    vector<int> initial_indices(nb_trains, 0);
    bool done_initial = false;
    while (!done_initial)
    {
        // map cyclical routes done by each train
        vector<int> cyclic_indices(nb_trains, 0);
        bool done_cyclic = false;
        while (!done_cyclic)
        {
            for (int i = 0; i < data.get_max_nb_trips(); i++) 
            {
                vector<vector<int>> combination;
                bool valid_combination = true;
                for (int t = 0; t < nb_trains; t++)
                {
                    vector<int> routes_of_train;

                    // choose initial trip
                    routes_of_train.push_back(initial_valid_routes_set[initial_indices[t]]);

                    // choose cyclical route
                    for (int k = 0; k < i; k++)
                    {
                        if (k >= data.get_train_max_trips(t)-1)
                            break;

                        routes_of_train.push_back(cyclical_routes_set[cyclic_indices[t]]);
                    }

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
                if (valid_combination && verify_demands(data, combination) && normalize_candidates(data, combination))
                {
                    candidate_combinations.push_back(combination);
                    break;
                }
            }

            if (!candidate_combinations.empty() && candidate_combinations.back()[0].size() == 1)
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

    // for (int i = 0; i < candidate_combinations.size(); i++)
    // {
    //     cout << "Combination " << i+1 << ": " << endl;
    //     for (int j = 0; j < candidate_combinations[i].size(); j++)
    //     {
    //         cout << "Train " << j << ": ";
    //         for (int k = 0; k < candidate_combinations[i][j].size(); k++)
    //         {
    //             cout << candidate_combinations[i][j][k] << " ";
    //         }
    //         cout << endl;
    //     }
    //     cout << endl;
    // }
}

void Heuristic::create_subsets (Data &data, vector<vector<vector<int>>> &aux_candidates, vector<vector<int>> &current, int max_nb_trips)
{
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
                if (current[i].size() < max_nb_trips)
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

        if (valid && verify_demands(data, aux_combination) && normalize_candidates(data, aux_combination))
        {
            aux_candidates.push_back(aux_combination);
        }
    }
}

bool Heuristic::remove_trips (Data &data, bool remove_from_all_candiates, vector<vector<int>> &current, int iter)
{
    vector<vector<vector<int>>> aux_candidates;
    if (remove_from_all_candiates)
    {
        aux_candidates.clear();
        for (int i = 0; i < candidate_combinations.size(); i++)
        {
            create_subsets(data, aux_candidates, candidate_combinations[i], data.get_max_nb_trips()-iter);
        }
    }
    else
    {
        aux_candidates.clear();
        create_subsets(data, aux_candidates, current, data.get_max_nb_trips()-iter);
    }

    cout << endl << "Removing trips..." << endl << endl;
    // for (int i = 0; i < candidate_combinations.size(); i++)
    // {
    //     cout << "Combination " << i+1 << ": " << endl;
    //     for (int j = 0; j < candidate_combinations[i].size(); j++)
    //     {
    //         cout << "Train " << j << ": ";
    //         for (int k = 0; k < candidate_combinations[i][j].size(); k++)
    //         {
    //             cout << candidate_combinations[i][j][k] << " ";
    //         }
    //         cout << endl;
    //     }
    //     cout << endl;
    // }

    // for (int i = 0; i < aux_candidates.size(); i++)
    // {
    //     cout << "Combination " << i+1 << ": " << endl;
    //     for (int j = 0; j < aux_candidates[i].size(); j++)
    //     {
    //         cout << "Train " << j << ": ";
    //         for (int k = 0; k < aux_candidates[i][j].size(); k++)
    //         {
    //             cout << aux_candidates[i][j][k] << " ";
    //         }
    //         cout << endl;
    //     }
    //     cout << endl;
    // }

    // if there are no candidates, it means demands were not met
    if (aux_candidates.empty())
    {
        return false;
    }
    else
    {   candidate_combinations = aux_candidates;
        return true;
    }
}

bool Heuristic::add_trips (Data &data)
{
    cout << endl << "Adding trips to candidate combinations..." << endl;

    for (int i = 0; i < candidate_combinations.size(); i++)
    {
        for (int t = 0; t < data.get_nb_trains(); t++)
        {
            int route = candidate_combinations[i][t].back();

            if (candidate_combinations[i][t].size() < data.get_train_max_trips(t))
                candidate_combinations[i][t].push_back(route);
            else
                return false;
        }

    }

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
        cout << endl;
    }

    return true;
}

void Heuristic::change_trips (Data &data, bool change_all_candidates, vector<vector<int>> &current)
{
    // final trip is chosen by the model
    // obs.: -1 = free, nb_routes = trip not made

    cout << endl << "Relaxing trips, so the model can make changes..." << endl;

    if (change_all_candidates)
    {
        candidate_combinations.clear();
        int count = 0;
        for (const auto& tested : unique_combinations)
        {
            count++;
            vector<vector<int>> candidate = tested;

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
            cout << endl;
        }
    }
    else
    {
        // last route is free for the model to choose
        for (int i = 0; i < data.get_nb_trains(); i++)
        {
            if (current[i].empty())
            {
                current[i].push_back(-1);
                continue;
            }

            current[i].back() = -1;
        }

        // first route is free for the model to choose
        // for (int i = 0; i < data.get_nb_trains(); i++)
        // {
        //     current[i].front() = -1;
        // }
    }

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
        cout << "\t> Cannot solve current instance with heuristic method!" << endl;
        exit(0);

        // create_artificial_cycles(data);

        // for (int i = 0; i < cycles_of_routes.size(); i++)
        // {
        //     cyclical_routes_set.push_back(i+data.get_nb_routes());
        // }
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
    cout << endl << "Selecting set of initial routes...";

    // if (have_cycles)
    // {
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
    // }
    // else
    // {
    //     for (int i = 0; i < data.get_nb_routes(); i++)
    //     {
    //         if (data.is_valid_route(0, 0, i))
    //         {
    //             initial_valid_routes_set.push_back(i);
    //         }
    //     }

    //     for (int i = 0; i < cycles_of_routes.size(); i++)
    //     {
    //         int first_vertex = data.get_route_vertices(cycles_of_routes[i].front()).front();
    //         cout << data.get_vertex_point(first_vertex) << " == " << data._initial_point << endl;
    //         if (data.get_vertex_point(cycles_of_routes[i].front()) == data._initial_point)
    //             initial_valid_routes_set.push_back(i+data.get_nb_routes());
    //     }
    // }

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

// bool compare_pair(const pair<int,int> &a, const pair<int,int> &b) {
//     return a.second > b.second;
// }

// void Heuristic::create_artificial_cycles(Data &data)
// {
//     // get the extreme points as start
//     vector <int> start = {0, data.get_nb_points()-1};

//     // starting from each extreme point
//     int next_point;
//     int point_for_reverse;

//     for (int i = 0; i < start.size(); i++)
//     {
//         vector <int> cycle;

//         next_point = start[i];
//         if (i == 0)
//             point_for_reverse = start[1];
//         else
//             point_for_reverse = start[0];

//         vector <int> routes_considered;
//         for (int i = 0; i < data.get_nb_routes(); i++)
//         {   
//             routes_considered.push_back(i);
//         }
    
//         // store index and size of routes
//         vector <vector <pair <int, int>>> candidate_subsequent_routes;    
//         bool reversed = false;
//         int iter = 0;
//         while (true)
//         {
//             vector <pair <int, int>> next_routes;
    
//             int same_direction, opposite_direction;
//             if (reversed)
//             {
//                 if (i == 0)
//                 {
//                     opposite_direction = data.get_point_vertices(next_point)[0];
//                     same_direction = data.get_point_vertices(next_point)[1];
//                 }
//                 else
//                 {
//                     opposite_direction = data.get_point_vertices(next_point)[1];
//                     same_direction = data.get_point_vertices(next_point)[0];
//                 }
//             }
//             else
//             {
//                 if (i == 0)
//                 {
//                     same_direction = data.get_point_vertices(next_point)[0];
//                     opposite_direction = data.get_point_vertices(next_point)[1];
//                 }
//                 else
//                 {
//                     same_direction = data.get_point_vertices(next_point)[1];
//                     opposite_direction = data.get_point_vertices(next_point)[0];
//                 }
//             }
    
//             bool found_connection = false;
    
//             if (next_point == point_for_reverse)
//             {
//                 for (auto route : routes_considered)
//                 {
//                     if (data.get_route_vertices(route)[0] == same_direction || data.get_route_vertices(route)[0] == opposite_direction)
//                     {
//                         if (data.get_route_vertices(route)[0] == opposite_direction)
//                             reversed = true;
//                         found_connection = true;
//                         next_routes.push_back({route, data.get_route_vertices(route).size()});
//                     }
//                 }
//             }
//             else
//             {
//                 for (auto route : routes_considered)
//                 {
//                     if (data.get_route_vertices(route)[0] == same_direction)
//                     {
//                         found_connection = true;
//                         next_routes.push_back({route, data.get_route_vertices(route).size()});
//                     }
//                 }
//             }
    
//             int chosen_route;
//             if (found_connection == false)
//             {
//                 bool failed_to_form_cycle = false;
//                 bool entered_loop = false;
//                 while (candidate_subsequent_routes[iter].size() < 2) // while we have only one or less option
//                 {
//                     entered_loop = true;
//                     if (candidate_subsequent_routes[iter].size() == 1)
//                     {
//                         candidate_subsequent_routes[iter].erase(candidate_subsequent_routes[iter].begin());
//                         candidate_subsequent_routes.erase(candidate_subsequent_routes.begin() + iter);

//                         // add to considered and delete from cycle
//                         routes_considered.push_back(cycle[iter]);
//                         cycle.erase(cycle.begin() + iter);
//                     }
//                     iter--;

//                     if (iter < 0)
//                         failed_to_form_cycle = true;
//                 }

//                 if (failed_to_form_cycle)
//                     break;

//                 if (!entered_loop)
//                     iter--;
                
//                 // delete previously chosen
//                 candidate_subsequent_routes[iter].erase(candidate_subsequent_routes[iter].begin());

//                 // add to considered and delete from cycle
//                 routes_considered.push_back(cycle[iter]);
//                 cycle.erase(cycle.begin() + iter);
    
//                 // add new chosen to the cycle and delete from considered
//                 next_routes = candidate_subsequent_routes[iter];
//                 chosen_route = next_routes[0].first;
    
//                 cycle.push_back(chosen_route);
//                 routes_considered.erase(remove(routes_considered.begin(), routes_considered.end(), chosen_route));
//                 next_point = data.get_vertex_point(data.get_route_vertices(chosen_route).back());
    
//                 continue;
//             }
    
//             sort(next_routes.begin(), next_routes.end(), compare_pair);
//             chosen_route = next_routes[0].first;
            
//             candidate_subsequent_routes.push_back(next_routes);
    
//             cycle.push_back(chosen_route);
//             routes_considered.erase(remove(routes_considered.begin(), routes_considered.end(), chosen_route));
//             next_point = data.get_vertex_point(data.get_route_vertices(chosen_route).back());
    
//             if (next_point == start[i])
//                 break;
    
//             iter++;
//         }

//         if (!cycle.empty())
//             cycles_of_routes.push_back(cycle);
//     }

//     if (cycles_of_routes.empty())
//     {
//         cout << "Couldn't form a full cycle!" << endl;
//         exit(1);
//     }
// }

void Heuristic::try_new_set_of_routes(Data &data, int iter_set)
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

    // adicionar rotas não cíclicas completas

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

    // candidate_combinations.clear();
    create_maximum_size_candidates(data);
}