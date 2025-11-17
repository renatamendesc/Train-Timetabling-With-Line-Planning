#include "Solution.hpp"

using namespace std;

CombinationMatrix Solution::get_combination()
{
    for (int i = 0; i < data.get_nb_trains(); i++)
    {
        vector <int> aux;
        for (int j = 0; j < data.get_train_max_trips(i); j++)
        {
            for (int k = 0; k < data.get_nb_routes(); k++)
            {
                if (lambda_values[i][j][k] == 1)
                {
                    aux.push_back(k);
                    break;
                } 
            }
        }
        routes_combination.push_back(aux);
    }
}

int Solution::get_max_nb_repeated_route()
{
    // get the amount of times each route is completed
    vector <int> times_route_is_completed (data.get_nb_routes(), 0);

    // for each train
    for (int j = 0; j < combinations[i].size(); j++)
    {
        // for each trip
        for (int k = 0; k < combinations[i][j].size(); k++)
        {   
            int route = combinations[i][j][k];
            if (route != data.get_nb_routes())
            {
                times_route_is_completed[route]++;
            }
        }
    }
    max_nb_repeated_route = 0;
    for (int i = 0; i < times_route_is_completed.size(); i++)
    {
        if (times_route_is_completed[i] > max_nb_repeated_route)
            max_nb_repeated_route = times_route_is_completed[i];
    }
}

void Solution::get_solution(Data &data)
{
    cout << endl << "-> Solution value = " << obj_value << endl;
    cout << "-> Total time = " << computational_time.count() << endl;
    // pending: get gap value

    if (proven_optimal)
    {
        cout << "\tSolution is proven optimal!" << endl;
    }
    else
    {
        cout << "\tSolution is not proven optimal!" << endl;
    }

    for (int t = 0; t < data.get_nb_trains(); t++)
    {
        cout << "=============" << endl
                << "Train " << t << endl
                << "=============" << endl;
        for (int i = 0; i < data.get_train_max_trips(t); i++)
        {
            for (int r = 0; r < data.get_nb_routes(); r++)
            {
                if (data.is_valid_route(t, i, r))
                {
                    if (lambda_values_[t][i][r] > 0)
                    {
                        cout << "> Trip " << i << endl;
                        int departure, arrival;

                        for (auto arc : data.get_route_arcs(r))
                        {
                            departure = arc.out;
                            arrival = arc.inc;

                            cout << "   " << departure << "(time " << y_values_[t][i][departure] << " - " << convert_time(y_values_[t][i][departure]) << ")"
                                    << "(time " << y_bar_values_[t][i][arrival] << " - " << convert_time(y_bar_values_[t][i][arrival])
                                    << ") -> ";
                        }
                        cout << arrival << endl;
                    }
                }
            }
        }
    }
}

string Solution::convert_time(int seconds)
{
    int hours = 0, minutes = 0;
    while (seconds >= 3600)
    {
        int time = seconds / 3600;
        seconds = seconds % 3600;

        hours += time;
    }
    while (seconds >= 60)
    {
        int time = seconds / 60;
        seconds = seconds % 60;

        minutes += time;
    }

    if (hours < 10)
    {
        if (seconds < 10)
        {
            if (minutes < 10)
                return string("0" + to_string(hours) + ":0" + to_string(minutes) + ":0" + to_string(seconds));
            else
                return string("0" + to_string(hours) + ":" + to_string(minutes) + ":0" + to_string(seconds));
        }
        else
        {
            if (minutes < 10)
                return string("0" + to_string(hours) + ":0" + to_string(minutes) + ":" + to_string(seconds));
            else
                return string("0" + to_string(hours) + ":" + to_string(minutes) + ":" + to_string(seconds));
        }
    }
    else
    {
        if (seconds < 10)
        {
            if (minutes < 10)
                return string(to_string(hours) + ":0" + to_string(minutes) + ":0" + to_string(seconds));
            else
                return string(to_string(hours) + ":" + to_string(minutes) + ":0" + to_string(seconds));
        }
        else
        {
            if (minutes < 10)
                return string(to_string(hours) + ":0" + to_string(minutes) + ":" + to_string(seconds));
            else
                return string(to_string(hours) + ":" + to_string(minutes) + ":" + to_string(seconds));
        }
    }
}