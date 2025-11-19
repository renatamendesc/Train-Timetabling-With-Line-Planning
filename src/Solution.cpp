#include "Solution.hpp"

using namespace std;

void Solution::store_combination(Data &data)
{
    routes_combination.clear();
    for (int i = 0; i < data.get_nb_trains(); i++)
    {
        vector <int> aux;
        for (int j = 0; j < data.get_train_max_trips(i); j++)
        {
            for (int k = 0; k < data.get_nb_routes(); k++)
            {
                if (lambda_values_[i][j][k] == 1)
                {
                    aux.push_back(k);
                    break;
                } 
            }
        }
        routes_combination.push_back(aux);
    }
}

void Solution::store_max_nb_repeated_route(Data &data)
{
    // get the amount of times each route is completed
    vector <int> times_route_is_completed (data.get_nb_routes(), 0);

    // for each train
    for (int i = 0; i < routes_combination.size(); i++)
    {
        // for each trip
        for (int j = 0; j < routes_combination[i].size(); j++)
        {   
            int route = routes_combination[i][j];
            if (route != data.get_nb_routes())
            {
                times_route_is_completed[route]++;
            }
        }
    }
    max_nb_repeated_routes = 0;
    for (int i = 0; i < times_route_is_completed.size(); i++)
    {
        if (times_route_is_completed[i] > max_nb_repeated_routes)
            max_nb_repeated_routes = times_route_is_completed[i];
    }
}

void Solution::display_routes_combination(Data &data)
{
    cout << "Routes combination:" << endl;
    for (int i = 0; i < routes_combination.size(); i++)
    {
        cout << "Train " << i << ": ";
        for (int j = 0; j < routes_combination[i].size(); j++)
        {
            cout << routes_combination[i][j] << " ";
        }
        cout << endl;
    }
}

void Solution::display_solution(Data &data)
{
    cout << "-> Solution value = " << obj_value << endl;
    // pending: get gap value

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