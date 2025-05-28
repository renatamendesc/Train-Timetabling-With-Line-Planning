#include <Heuristic.hpp>

using namespace std;

// -> Real instance:
// #num_routes
// 6

// #routes
// 0    1    2    3    4    5    6    7    -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1
// 23   22   21   20   19   18   17   16   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1
// 7    8    9    10   11   12   13   14   15   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1
// 31   30   29   28   27   26   25   24   23   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1   -1
// 0    1    2    3    4    5    6    7    8    9    10   11   12   13   14   15   31   30   29   28   27   26   25   24   23   22   21   20   19   18   17   16    0  
// 31   30   29   28   27   26   25   24   23   22   21   20   19   18   17   16   0    1    2    3    4    5    6    7    8    9    10   11   12   13   14   15   31  

void Heuristic::create_initial_combinations (Data &data)
{
    create_cyclical_routes_set(data);
    create_initial_valid_routes_set(data);

    int nb_trains = data.get_nb_trains();
    int num_valid = initial_valid_routes_set.size();
    int num_cyclic = cyclical_routes_set.size();

    // for (int i = 0; i < cyclical_routes_set.size(); i++)
    // {
    //     vector<vector<int>> combination (nb_trains);
    //     for (int j = 0; j < nb_trains; j++)
    //     {
    //         cout << "Trem " << j << " faz " << data.get_train_max_trips(j) << " trips" << endl;
    //         for (int k = 0; k < data.get_train_max_trips(j); k++)
    //         {
    //             combination[j].push_back(cyclical_routes_set[i]);
    //             cout << "adicionei..." << endl;
    //         }
    //     }
    //     candidate_combinations.push_back(combination);
    // }

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

                // chose initial trip
                routes_of_train[0] = initial_valid_routes_set[initial_indices[t]];

                // chose cyclical route
                for (int k = 1; k < nb_trips; k++)
                {
                    routes_of_train[k] = cyclical_routes_set[cyclic_indices[t]];
                }
                combination.push_back(routes_of_train);
            }
            candidate_combinations.push_back(combination);

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

    // add_all_subsets(data); // modifcar para remover o tamanho até demandas não serem atendidas

    for (int i = 0; i < candidate_combinations.size(); i++)
    {
        cout << "Combinação " << i+1 << ": " << endl;
        for (int j = 0; j < candidate_combinations[i].size(); j++)
        {
            cout << "Trem " << j << ": ";
            for (int k = 0; k < candidate_combinations[i][j].size(); k++)
            {
                cout << candidate_combinations[i][j][k] << " ";
            }
            cout << endl;
        }
        cout << endl;
    }
}

void Heuristic::add_all_subsets (Data &data)
{
    int initial_size = candidate_combinations.size();
    for (int i = 0; i < initial_size; i++)
    {
        vector<vector<int>> combination = candidate_combinations[i];

        int nb_trains = combination.size();
        vector<int> max_lengths(nb_trains);
        for (int i = 0; i < nb_trains; i++)
            max_lengths[i] = data.get_train_max_trips(i);
    
        vector<int> current_lengths = max_lengths;
        bool done = false;
    
        while (!done)
        {
            // Criar subconjunto com os tamanhos atuais
            vector<vector<int>> subset;
            for (int i = 0; i < nb_trains; i++) {
                vector<int> reduced(combination[i].begin(),
                                    combination[i].begin() + current_lengths[i]);
                subset.push_back(reduced);
            }

            for (int i = 0; i < subset.size(); i++)
            {
                while (subset[i].size() < data.get_train_max_trips(i))
                    subset[i].push_back(data.get_nb_routes());
            }
    
            // Armazenar no vetor de subconjuntos e também nos candidatos
            candidate_combinations.push_back(subset);
    
            // Atualizar os comprimentos, como um contador misto
            for (int i = nb_trains - 1; i >= 0; i--)
            {
                if (current_lengths[i] > 1)
                {
                    current_lengths[i]--;
                    for (int j = i + 1; j < nb_trains; j++)
                        current_lengths[j] = max_lengths[j];
                    break;
                }
                else if (i == 0)
                {
                    done = true;
                }
            }
        }
    }
}

void Heuristic::remove_trips (Data &data, bool feasible, vector<vector<int>> current, int min_nb_trips)
{

    // Gerar todos os prefixos de cada linha (do maior para o menor)
    std::vector<std::vector<std::vector<int>>> all_prefixes;
    for (const auto& row : current) {
        std::vector<std::vector<int>> prefixes;
    
        // Aqui está o truque: começamos de row.size() - 1
        for (int len = static_cast<int>(row.size()) - 1; len >= 0; --len) {
            prefixes.emplace_back(row.begin(), row.begin() + len);
        }
    
        all_prefixes.push_back(prefixes);
    }

    const auto& row1_prefixes = all_prefixes[0];
    const auto& row2_prefixes = all_prefixes[1];

    candidate_combinations.clear();

    cout << "Tamanho mínimo: " << min_nb_trips << endl;

    // Produto cartesiano com filtro de tamanho total
    for (const auto& prefix1 : row1_prefixes) {
        for (const auto& prefix2 : row2_prefixes) {
            if (prefix1.size() >= min_nb_trips && prefix2.size() >= min_nb_trips) {
                candidate_combinations.push_back({prefix1, prefix2});
            }
        }
    }

    // // Exibir os resultados
    // for (const auto& submatrix : candidate_combinations) {
    //     std::cout << "{\n";
    //     for (const auto& row : submatrix) {
    //         std::cout << "  { ";
    //         for (int val : row)
    //             std::cout << val << " ";
    //         std::cout << "}\n";
    //     }
    //     std::cout << "}\n\n";
    // }

    // if (feasible)
    // {
        // candidate_combinations.clear();

        // vector<int> max_lengths(data.get_nb_trains());
        // for (int i = 0; i < data.get_nb_trains(); i++)
        //     max_lengths[i] = current[i].size();

        // vector<int> current_lengths = max_lengths;
        // bool done = false;
        // while (!done)
        // {
        //     vector<vector<int>> subset;
        //     for (int i = 0; i < data.get_nb_trains(); i++)
        //     {
        //         vector<int> reduced(current[i].begin(), current[i].begin() + current_lengths[i]);
        //         subset.push_back(reduced);
        //     }
        //     candidate_combinations.push_back(subset);

        //     for (int i = data.get_nb_trains()-1; i >= 0; i--)
        //     {
        //         if (current_lengths[i] > current[i].size()-1)
        //         {
        //             current_lengths[i]--;
        //             for (int j = i + 1; j < data.get_nb_trains(); j++)
        //                 current_lengths[j] = max_lengths[j];
        //             break;
        //         }
        //         else if (i == 0)
        //         {
        //             done = true;
        //         }
        //     }
        // } 
    // }
    // else
    // {

    // }

    cout << "Flexibilizando viagens..." << endl << endl;
    for (int i = 0; i < candidate_combinations.size(); i++)
    {
        cout << "Combinação " << i+1 << ": " << endl;
        for (int j = 0; j < candidate_combinations[i].size(); j++)
        {
            cout << "Trem " << j << ": ";
            for (int k = 0; k < candidate_combinations[i][j].size(); k++)
            {
                cout << candidate_combinations[i][j][k] << " ";
            }
            cout << endl;
        }
        cout << endl;
    }
}

void Heuristic::change_trips (Data &data, vector<vector<int>> &current)
{
    // final trip is chosen by the model
    // obs.: -1 = free, nb_routes = trip not made

    for (int i = 0; i < data.get_nb_trains(); i++)
    {
        current[i].back() = -1;
    }

    // current[1].back() = 7;

    cout << endl;
    for (int i = 0; i < current.size(); i++)
    {
        cout << "Trem " << i+1 << ": ";
        for (int j = 0; j < current[i].size(); j++)
        {
            cout << current[i][j] << " ";
        }
        cout << endl;
    }

}

void Heuristic::create_cyclical_routes_set(Data &data)
{
    for (int i = 0; i < data.get_nb_routes(); i++)
    {
        if (data.is_cyclic_route(i))
            cyclical_routes_set.push_back(i);
    }

    cout << "Rotas cíclicas: " << endl;
    for (int i  = 0; i < cyclical_routes_set.size(); i++)
    {
        cout << cyclical_routes_set[i] << " ";
    }
    cout << endl;
}

void Heuristic::create_initial_valid_routes_set(Data &data)
{
    for (int i = 0; i < data.get_nb_routes(); i++)
    {
        if (data.is_valid_route(0, 0, i))
            initial_valid_routes_set.push_back(i);
    }

    cout << "Rotas iniciais válidas: " << endl;
    for (int i  = 0; i < initial_valid_routes_set.size(); i++)
    {
        cout << initial_valid_routes_set[i] << " ";
    }
    cout << endl;
}