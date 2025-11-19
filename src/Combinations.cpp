#include <Combinations.hpp>

using namespace std;

int Combinations::calculate_trips_combinations(Data &data)
{
    int nb_trains = data.get_nb_trains();
    int nb_routes = data.get_nb_routes();

    all_trips_combinations.resize(nb_trains);
    for (int i = 0; i < nb_trains; i++)
    {
        // verify whether trips were already calculated for the train
        bool skip = false;
        for (int j = i-1; j >= 0; j--)
        {
            if (data.get_train_max_trips(j) == data.get_train_max_trips(i))
            {
                all_trips_combinations[i] = all_trips_combinations[j];
                skip = true;
                break;
            }
        }
        if (!skip)
        {
            if (verify_overflow(nb_routes, data.get_train_max_trips(i)))
            {
                // error: too many combinations!
                return 0;
            }
            int nb_trips_comb_for_train = pow(nb_routes+1, data.get_train_max_trips(i)); // using nb_routes+1 to consider that no route is made
            
            // calculate number of trips combinations
            generate_trips_combinations(data, i);
        }
    }
    return 1;
}

void Combinations::generate_trips_combinations(Data &data, int train_idx)
{
    int nb_routes = data.get_nb_routes();
    int nb_trips_comb_for_train = pow(nb_routes+1, data.get_train_max_trips(train_idx));

    vector<int> current(data.get_train_max_trips(train_idx), 0);
    for (unsigned long long count = 0; count < nb_trips_comb_for_train; count++)
    {
        // verify feasibility of the current combination before adding to the vector
        if (check_trips_feasibility(data, current))
        {
            all_trips_combinations[train_idx].push_back(current);
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

bool Combinations::is_valid_combination (Data &data, RoutesCombination &combination)
{
    if (!normalize_combination(data, combination))
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
    for (int i = 0; i < combination.size(); i++)
    {
        for (int j = 0; j < combination[i].size(); j++)                        
        {
            if (combination[i][j] != data.get_nb_routes() && combination[i][j] != -1)
            {
                int route = combination[i][j];
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


bool Combinations::check_trips_feasibility (Data &data, vector<int> &sequence_of_trips)
{
    int nb_routes = data.get_nb_routes();
    
    // verify whether first route starts at the initial depot
    if (sequence_of_trips[0] != nb_routes)
    {
        if (!data.is_valid_route(0, 0, sequence_of_trips[0])) return false;
    }

    bool flag = false; // flag to tell whether train completes any trips during the day
    for (int i = 0; i < sequence_of_trips.size()-1; i++)
    {
        if (sequence_of_trips[i] != nb_routes)
        {
            flag = true;
            if (sequence_of_trips[i+1] != nb_routes)
            {
                // verify whether subsequential routes are compatible
                if (data.are_incompatible_routes(sequence_of_trips[i], sequence_of_trips[i+1])) return false;
            }
        }
        else if (sequence_of_trips[i] == nb_routes)
        {
            // if trip is not made, verify whether the next ones also are not made
            if (i != sequence_of_trips.size()-1 && sequence_of_trips[i+1] != nb_routes) return false;
        }
    }

    if (sequence_of_trips.front() != nb_routes) flag = true;

    return flag; 
}

bool Combinations::normalize_combination (Data &data, RoutesCombination &combination)
{
    // normalize combinations to verify whether it was already tested previously
    // (only changes the train that will complete the trips)
    // return false if it was already tested and returns true if it's a new combination

    int nb_routes = data.get_nb_routes();
    // make sure all dimensions have the same size
    for (int i = 0; i < combination.size(); i++)
    {
        while (combination[i].size() < data.get_max_nb_trips())
            combination[i].push_back(nb_routes);
    }
    // normalize the vector
    vector<vector<int>> normalized_combination = combination;
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





