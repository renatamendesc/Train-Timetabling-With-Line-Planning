#include <Combinations.hpp>

using namespace std;

void Combinations::init(Data &data)
{
    // calculate number of trips combinations
    nb_routes = data.get_nb_routes()+1;                    // sums 1 to consider the possibility that the trip might not be made
    max_nb_trips = data.get_max_nb_trips();
    nb_trips_combinations = pow(nb_routes, max_nb_trips);

    generate_trips_combinations();
    cout << endl;
    for (int i = 0; i < trips_combinations.size(); i++)
    {
        cout << "Combination " << i << ": " << endl;
        for (int j = 0; j < trips_combinations[i].size(); j++)
        {
            cout << trips_combinations[i][j] << " " << endl;;
        }
        cout << endl;
    }

    // calculate total number of possible combinations
    nb_trains = data.get_nb_trains();
    total_nb_combinations = pow(nb_trips_combinations, nb_trains); 

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
    vector<int> current(nb_trains, 0); // current selection for the trains

    for (unsigned long long count = 0; count < total_nb_combinations; count++)
    {
        // display current combination
        all_combinations.push_back(current);

        // go to next combination
        for (int i = nb_trains-1; i >= 0; i--)
        {
            if (++current[i] < nb_trips_combinations)
                break;
            current[i] = 0;
        }
    }
}

void Combinations::generate_trips_combinations()
{
    vector<int> current(max_nb_trips, 0);

    for (unsigned long long count = 0; count < nb_trips_combinations; count++)
    {
        // display current combination
        trips_combinations.push_back(current);

        // go to next combination
        for (int i = max_nb_trips-1; i >= 0; i--)
        {
            if (++current[i] < nb_routes)
                break;
            current[i] = 0;
        }
    }
}







