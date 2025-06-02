#include <Heuristic.hpp>

using namespace std;

void Heuristic::create_initial_combinations (Data &data)
{
    cout << endl << "Creating initial set of candidate combinations..." << endl;

    create_cyclical_routes_set(data);
    create_initial_valid_routes_set(data);

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
                combination.push_back(routes_of_train);
            }
            candidate_combinations.push_back(combination);

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
    cout << "Creating subsets from initial candidate combinations..." << endl;

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
    cout << "Removing trips from current best combination..." << endl;

    bool nb_trips_met_demand = true;

    candidate_combinations.clear();
    std::vector<std::vector<std::vector<int>>> resultado;

    size_t totalCombinacoes = 1 << data.get_nb_trains();  // 2^n combinações possíveis

    for (size_t mask = 0; mask < totalCombinacoes; ++mask) {
        if (mask == 0) continue;
        std::vector<std::vector<int>> combinacao;

        bool valida = true;

        for (size_t i = 0; i < data.get_nb_trains(); ++i) {
            // Se o bit i da máscara estiver ativado, tenta remover 1 elemento
            if ((mask >> i) & 1) {
                if (current[i].size() < min_nb_trips) {
                    valida = false;  // Não é possível remover elemento dessa linha
                    break;
                }
                combinacao.push_back(std::vector<int>(current[i].begin(), current[i].end() - 1));
            } else {
                combinacao.push_back(current[i]);  // Mantém linha original
            }
        }

        if (valida) {
            if (verify_demands(data, combinacao))
            {
                candidate_combinations.push_back(combinacao);   
                // cout << "cumpre demanda" << endl;
            }
            else
            {
                nb_trips_met_demand = false;
                // cout << "não cumpre demanda" << endl;

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

    return nb_trips_met_demand;
}

void Heuristic::change_trips (Data &data, vector<vector<int>> &current)
{
    // final trip is chosen by the model
    // obs.: -1 = free, nb_routes = trip not made

    cout << "Relaxing trips from current best combination, so the model can make changes..." << endl;

    // teste inicial: deixar livre as primeiras e ultimas viagens
    for (int i = 0; i < data.get_nb_trains(); i++)
    {
        current[i].back() = -1;
    }
    for (int i = 0; i < data.get_nb_trains(); i++)
    {
        current[i].front() = -1;
    }

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

bool Heuristic::verify_demands(Data &data, std::vector<std::vector<int>> &current)
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
    for (int i = 0; i < data.get_nb_routes(); i++)
    {
        if (data.is_cyclic_route(i))
            cyclical_routes_set.push_back(i);
    }

    // cout << "Rotas cíclicas: " << endl;
    // for (int i  = 0; i < cyclical_routes_set.size(); i++)
    // {
    //     cout << cyclical_routes_set[i] << " ";
    // }
    // cout << endl;
}

void Heuristic::create_initial_valid_routes_set(Data &data)
{
    for (int i = 0; i < data.get_nb_routes(); i++)
    {
        if (data.is_valid_route(0, 0, i))
            initial_valid_routes_set.push_back(i);
    }

    // cout << "Rotas iniciais válidas: " << endl;
    // for (int i  = 0; i < initial_valid_routes_set.size(); i++)
    // {
    //     cout << initial_valid_routes_set[i] << " ";
    // }
    // cout << endl;
}