#include <Combinations.hpp>

using namespace std;

void Combinations::init(Data &data)
{
    // calculate number of trips combinations
    nb_routes = data.get_nb_routes();                    
    max_nb_trips = data.get_max_nb_trips();
    max_nb_trips_combinations = pow(nb_routes+1, max_nb_trips); // sums 1 to consider the possibility that the trip might not be made

    generate_trips_combinations(data);
    cout << endl;
    for (int i = 0; i < trips_combinations.size(); i++)
    {
        cout << "(Trip) Combination " << i << ": " << endl;
        for (int j = 0; j < trips_combinations[i].size(); j++)
        {
            cout << trips_combinations[i][j] << " " << endl;;
        }
        cout << endl;
    }

    // calculate total number of possible combinations
    nb_trains = data.get_nb_trains();
    total_nb_combinations = pow(trips_combinations.size(), nb_trains); 

    generate_all_combinations();
    for (int i = 0; i < all_combinations.size(); i++)
    {
        cout << "Combination " << i << ": " << endl;
        for (int j = 0; j < all_combinations[i].size(); j++)
        {
            cout << "Train " << j << ": ";
            int idx_trip_comb = all_combinations[i][j];
            for (int k = 0; k < trips_combinations[idx_trip_comb].size(); k++)
            {
                cout << trips_combinations[idx_trip_comb][k] << " ";
            }
            cout << endl;
        }
        cout << endl;
    }
}

void Combinations::generate_all_combinations()
{
    vector<int> current(nb_trains, 0);
    for (unsigned long long count = 0; count < total_nb_combinations; count++)
    {
        // display current combination
        all_combinations.push_back(current);

        // go to next combination
        for (int i = nb_trains-1; i >= 0; i--)
        {
            if (++current[i] < trips_combinations.size())
                break;
            current[i] = 0;
        }
    }
}

bool check_final_feasibility (Data &data, std::vector <int> &current)
{

}

void Combinations::generate_trips_combinations(Data &data)
{
    vector<int> current(max_nb_trips, 0);
    for (unsigned long long count = 0; count < max_nb_trips_combinations; count++)
    {
        // verify feasibility of the current combination before adding to the vector
        if (check_trips_feasibility(data, current)) 
            trips_combinations.push_back(current);

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
    // verifies whether first route starts at the initial depot
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








