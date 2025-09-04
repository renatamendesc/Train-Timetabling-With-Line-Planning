#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <vector>
#include <utility>
#include <tuple>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <filesystem>

#include "../../include/Data.hpp"
#include "../../include/Model.hpp"
#include "../../include/Combinations.hpp"

using namespace std;

#define MIN_TIME 3600
#define MAX_TIME 64800

string set_name;

int min_points = 3, max_points = 10;
int min_trains, max_trains;

int std_arc;

int random_max_time ()
{
    if (set_name == string("instances-real"))
        return 43200 + rand() % (MAX_TIME - 43200 + 1); // minimum time must be at least 12 hours (43200 seconds)

    // random number from MIN_TIME to MAX_TIME
    return MIN_TIME + rand() % (MAX_TIME - MIN_TIME + 1);
}

int random_nb_intervals (int max_time)
{
    if (max_time < 10801)
    {
        return 1;
    }
    else if (max_time < 21601)
    {
        return 1 + rand() % 2;
    }
    else if (max_time < 32401)
    {
        return 1 + rand() % 3;
    }
    else if (max_time < 43201)
    {
        return 1 + rand() % 4;
    }
    else if (max_time < 54001)
    {
        return 1 + rand() % 5;
    }
    else 
    {
        return 1 + rand() % 5;
    }
}

void generate_time_intervals(int max_time, vector <tuple <int, int>> &time_intervals)
{
    int nb_intervals = random_nb_intervals(max_time);

    if (nb_intervals == 1)
    {
        time_intervals.push_back({0, max_time});
    }
    else if (nb_intervals == 2)
    {
        int interval = floor(max_time / 2);
        time_intervals.push_back({0, interval});
        time_intervals.push_back({interval+1, max_time});
    }
    else if (nb_intervals == 3)
    {
        // intervalo dividido igualmente
        int interval = floor(max_time / 3);
        time_intervals.push_back({0, interval});
        time_intervals.push_back({interval+1, interval*2});
        time_intervals.push_back({(interval*2)+1, max_time});
    }
    else if (nb_intervals == 4)
    {
        // intervalo dividido igualmente
        int interval = floor(max_time / 4);
        time_intervals.push_back({0, interval});
        time_intervals.push_back({interval+1, interval*2});
        time_intervals.push_back({(interval*2)+1, interval*3});
        time_intervals.push_back({(interval*3)+1, max_time});
    }
    else if (nb_intervals == 5)
    {
        // intervalo dividido igualmente
        int interval = floor(max_time / 5);
        time_intervals.push_back({0, interval});
        time_intervals.push_back({interval+1, interval*2});
        time_intervals.push_back({(interval*2)+1, interval*3});
        time_intervals.push_back({(interval*3)+1, interval*4});
        time_intervals.push_back({(interval*4)+1, max_time});
    }
    else
    {
        // intervalo dividido igualmente
        int interval = floor(max_time / 6);
        time_intervals.push_back({0, interval});
        time_intervals.push_back({interval+1, interval*2});
        time_intervals.push_back({(interval*2)+1, interval*3});
        time_intervals.push_back({(interval*3)+1, interval*4});
        time_intervals.push_back({(interval*4)+1, interval*5});
        time_intervals.push_back({(interval*5)+1, max_time});
    }
}

void generate_service_and_headway(int max_time, vector <int> &points, vector <int> &depots, vector <int> &crossings, vector <int> &stations, vector <int> &service_time_min, vector <int> &service_time_max, int &headway)
{
    // service time min
    pair <int, int> range_service_min_depot = {90, 300};
    pair <int, int> range_service_min_stations = {60, 120};
    pair <int, int> range_service_min_crossings = {30, 90};

    if (max_time < 10800)
    {
        range_service_min_depot = {90, 120};
        range_service_min_stations = {60, 90};
        range_service_min_crossings = {30, 60};
    }

    int service_min_depot = range_service_min_depot.first + rand() % (range_service_min_depot.second - range_service_min_depot.first + 1);
    if (service_min_depot < range_service_min_stations.second)
        range_service_min_stations.second = service_min_depot;

    int service_min_stations = range_service_min_stations.first + rand() % (range_service_min_stations.second - range_service_min_stations.first + 1);
    if (service_min_stations < range_service_min_crossings.second)
        range_service_min_crossings.second = service_min_stations;

    int service_min_crossings = range_service_min_crossings.first + rand() % (range_service_min_crossings.second - range_service_min_crossings.first + 1);
    
    // assign service time min
    for (auto c : crossings)
    {
        service_time_min[c] = service_min_crossings;
    }
    for (auto s : stations)
    {
        service_time_min[s] = service_min_stations;
    }
    for (auto d : depots)
    {
        service_time_min[d] = service_min_depot;
    }

    // service time max
    pair <int, int> range_service_max = {300, 600};
    int service_max = range_service_max.first + rand() % (range_service_max.second - range_service_max.first + 1);
    service_time_max.assign(points.size(), service_max);

    // headway
    pair <int, int> range_headway = {40, 90};
    headway = range_headway.first + rand() % (range_headway.second - range_headway.first + 1);

}

int random_nb_points ()
{
    return min_points + rand() % (max_points - min_points + 1);
}

void random_depots (vector <int> &points, vector <int> &depots)
{
    int nb_points = points.size();
    vector <int> candidate_list = points;

    // add essential depots to the list
    depots = {0, nb_points-1};

    if (candidate_list.size() >= 2)
    {
        candidate_list.pop_back();              
        candidate_list.erase(candidate_list.begin()); 
    }
    else
    {   
        candidate_list.clear();
    }

    // randomize number of extra depots
    int extra_depots;
    if (nb_points < 11)
    {
        extra_depots = rand() % 2;
    }
    else if (nb_points < 31)
    {
        extra_depots = rand() % 3;
    }
    else
    {
        extra_depots = rand() % 4;
    }

    // for real instances: add another depot on the middle
    if (set_name == string("instances-real"))
        extra_depots = 1;

    // add the extra depots to the list
    for (int i = 0; i < extra_depots; i++)
    {
        int idx = rand() % candidate_list.size();
        depots.push_back(candidate_list[idx]);
        candidate_list.erase(candidate_list.begin() + idx); 
    }
}

void random_crossings_and_stations (vector <int> &points, vector <int> &depots, vector <int> &crossings, vector <int> &stations)
{
    int nb_points = points.size();
    vector <int> candidate_list = points;

    // add essential crossings and stations
    for (auto p : depots)
    {
        crossings.push_back(p);
        stations.push_back(p);
        // delete depot added from candidate list
        for (int i = 0; i < candidate_list.size(); i++)
        {
            if (p == candidate_list[i])
            {
                candidate_list.erase(candidate_list.begin() + i);
                break;
            }
        }
    }

    // randomize number of extra crossings
    int extra_isolated_crossings;
    int extra_stations_crossings;
    if (nb_points < 6)
    {
        extra_isolated_crossings = rand() % 2;
        extra_stations_crossings = rand() % 2;
    }
    else if (nb_points < 11)
    {
        extra_isolated_crossings = rand() % 3;
        extra_stations_crossings = rand() % 3;
    }
    else if (nb_points < 21)
    {
        extra_isolated_crossings = rand() % 4;
        extra_stations_crossings = rand() % 4;
    }
    else if (nb_points < 41)
    {
        extra_isolated_crossings = rand() % 5;
        extra_stations_crossings = rand() % 5;
    }
    else
    {
        extra_isolated_crossings = rand() % 6;
        extra_stations_crossings = rand() % 6;
    }

    // add the extra isolated crossings to the list
    for (int i = 0; i < extra_isolated_crossings; i++)
    {
        int idx = rand() % candidate_list.size();
        crossings.push_back(candidate_list[idx]);
        candidate_list.erase(candidate_list.begin() + idx); 
    }

    // add the extra stations crossings to the list (including stations)
    for (int i = 0; i < extra_stations_crossings; i++)
    {
        int idx = rand() % candidate_list.size();
        crossings.push_back(candidate_list[idx]);
        stations.push_back(candidate_list[idx]);
        candidate_list.erase(candidate_list.begin() + idx); 
    }

    // add the stations that are left
    for (auto s : candidate_list)
    {
        stations.push_back(s);
    }
}

void generate_network (vector <int> &points, vector <int> &depots, vector <int> &crossings, vector <int> &stations)
{
    random_depots(points, depots);
    random_crossings_and_stations(points, depots, crossings, stations);
}

int random_initial_depot (vector <int> &depots) 
{
    return depots[rand() % depots.size()];
}

void generate_arcs (int max_time, int nb_points, vector <int> &depots, vector<vector<int>> &adj_matrix)
{
    // repensar calculo dos arcos? ----------------------
    int aproximate_nb_cycles = 1 + rand() % 5;
    int arc_max = floor((max_time / ((nb_points-1)*2)) / aproximate_nb_cycles);
    int arc_min = floor((MIN_TIME / ((nb_points-1)*2)) / aproximate_nb_cycles);
    // --------------------------------------------------

    // calculate arc making sure that it will be small enough to complete more than a full cycle
    std_arc = ((arc_max + arc_min) / 2) / 2;
    // cout << "Arco padrão: " << std_arc << endl;

    // sortear uma margem para cada arco, e atribuir valores à matriz
    double percentual = 0.1;
    int margin_lower = std_arc * (1 - percentual);
    int margin_upper = std_arc * (1 + percentual);

    // create matrix
    for (int i = 0; i < nb_points-1; i++)
    {
        int current_arc = margin_lower + rand() % (margin_upper - margin_lower + 1);
        adj_matrix[i][i+1] = current_arc;
        adj_matrix[i+1+nb_points][i+nb_points] = current_arc;
    }
    for (auto i : depots)
    {
        adj_matrix[i][i+nb_points] = 0;
        adj_matrix[i+nb_points][i] = 0;
    }
}

void generate_routes (vector <vector <int>> &routes, vector <int> &depots, int nb_points)
{
    vector <int> route;

    // rotas cíclicas completas partindo das extremidades
    for (int i = 0; i < nb_points; i++)
        route.push_back(i);
    for (int i = nb_points-1; i >= 0; i--)
        route.push_back(i+nb_points);
    route.push_back(0);

    routes.push_back(route);

    route.clear();
    for (int i = nb_points-1; i >= 0; i--)
        route.push_back(i+nb_points);
    for (int i = 0; i < nb_points; i++)
        route.push_back(i);
    route.push_back((nb_points*2)-1);

    routes.push_back(route);

    // rotas não cíclicas que ligam depósitos do meio com extremidades
    for (int i = 2; i < depots.size(); i++)
    {
        route.clear();
        for (int j = depots[i]; j < nb_points; j++)
            route.push_back(j);
        routes.push_back(route);

        route.clear();
        for (int j = depots[i]; j >= 0; j--)
            route.push_back(j+nb_points);
        routes.push_back(route);

        route.clear();
        for (int j = 0; j <= depots[i]; j++)
            route.push_back(j);
        routes.push_back(route);

        route.clear();
        for (int j = nb_points-1; j >= depots[i]; j--)
            route.push_back(j+nb_points);
        routes.push_back(route);
    }

    // rotas não essenciais
    vector <vector <int>> extra_routes;
    for (int i = 2; i < depots.size(); i++)
    {
        route.clear();
        for (int j = depots[i]; j < nb_points; j++)
            route.push_back(j);
        for (int j = nb_points-1; j >= depots[i]; j--)
            route.push_back(j+nb_points);
        route.push_back(depots[i]);
        extra_routes.push_back(route);

        route.clear();
        for (int j = depots[i]; j >= 0; j--)
            route.push_back(j+nb_points);
        for (int j = 0; j <= depots[i]; j++)
            route.push_back(j);
        route.push_back(depots[i]+nb_points);
        extra_routes.push_back(route);
    }

    // não cíclicas que ligam depósitos adjacentes
    for (int i = 2; i < depots.size()-1; i++)
    {
        int depot_a, depot_b;
        if (depots[i] < depots[i+1])
        {
            depot_a = depots[i];
            depot_b = depots[i+1];
        }
        else
        {
            depot_a = depots[i+1];
            depot_b = depots[i]; 
        }

        route.clear();
        for (int j = depot_a; j <= depot_b; j++)
            route.push_back(j);
        extra_routes.push_back(route);

        route.clear();
        for (int j = depot_b; j >= depot_a; j--)
            route.push_back(j+nb_points);
        extra_routes.push_back(route);
    }

    int nb_extra_selected = rand() % (extra_routes.size() + 1);
    for (int i = 0; i < nb_extra_selected; i++)
    {
        int idx = rand() % extra_routes.size();
        routes.push_back(extra_routes[idx]);

        // removes from vector efficiently
        swap(extra_routes[idx], extra_routes.back());
        extra_routes.pop_back();
    }
}

int random_nb_trains (int nb_points)
{
    if (set_name == string("instances-real"))
        return min_trains + rand() % (max_trains - min_trains + 1);

    if (nb_points < 5)
    {
        min_trains = 2;
        max_trains = 3;
    }
    else if (nb_points < 10)
    {
        min_trains = 2;
        max_trains = 5;
    }
    else if (nb_points < 20)
    {
        min_trains = 2;
        max_trains = 6;
    }
    else
    {
        min_trains = 2;
        max_trains = 8;
    }

    return min_trains + rand() % (max_trains - min_trains + 1);
}

void generate_demands_and_nb_trips(vector <vector <int>> &demands, vector <int> &max_nb_trips_per_train, vector <int> &depots, vector <int> &crossings, vector <int> &stations, int nb_intervals, int nb_points, int max_time)
{
    int min_full_cycle = (std_arc * (nb_points-1))*2;

    int min_demand = floor(max_time / min_full_cycle);

    int daily_demand = min_demand;
    int nb_trains =  max_nb_trips_per_train.size();
    if (nb_trains > 1)
        daily_demand = min_demand * ceil(nb_trains / 2);

    // cout << "Demanda diária para todos os vértices: " << daily_demand << endl;

    if (daily_demand < nb_intervals)
        daily_demand = nb_intervals;
    int demand_interval = floor(daily_demand / nb_intervals);
    for (int i = 0; i < nb_intervals; i++)
    {
        for (auto s : stations)
        {
            if (s == nb_points-1)
            {
                demands[(nb_points-1)+nb_points][i] = demand_interval;
                continue;
            }
            else if (s == 0)
            {
                demands[0][i] = demand_interval;
                continue;
            }
            demands[s][i] = demand_interval;
            demands[s+nb_points][i] = demand_interval;
        }

        for (auto d : depots)
        {
            if (d == nb_points-1)
            {
                demands[(nb_points-1)+nb_points][i] = demand_interval;
                continue;
            }
            else if (d == 0)
            {
                demands[0][i] = demand_interval;
                continue;
            }
            demands[d][i] = demand_interval;
            demands[d+nb_points][i] = demand_interval;
        }
    }

    // set number of trips
    int trips = ceil(daily_demand / nb_trains)+1;

    max_nb_trips_per_train.assign(nb_trains, trips);
}

void print_instance (int nb_trains, vector <int> &max_nb_trips_per_train, vector <int> &points, vector <int> &depots, vector <int> &crossings, vector <int> &stations, int initial_depot, int max_time, vector <tuple<int, int>> &time_intervals, vector<vector<int>> &adj_matrix, vector<vector <int>> &routes, vector <vector <int>> &demands, vector <int> &service_time_min, vector <int> &service_time_max, int headway)
{
    cout << "Número de trens: " << nb_trains << endl;
    cout << "\tmax trips per trains: ";
    for(int i = 0; i < max_nb_trips_per_train.size(); i++)
        cout << max_nb_trips_per_train[i] << setw(6);
    cout << endl;
    
    cout << "Número de pontos: " << points.size() << endl << "\t";
    for (auto p : points)
    {
        cout << p << " ";
    }
    cout << endl;

    cout << "Número de depósitos: " << depots.size() << endl << "\t";
    for (auto d : depots)
    {
        cout << d << " ";
    }
    cout << endl;

    cout << "Número de cruzamentos: " << crossings.size() << endl << "\t";
    for (auto c : crossings)
    {
        cout << c << " ";
    }
    cout << endl;

    cout << "Número de estações: " << stations.size() << endl << "\t";
    for (auto s : stations)
    {
        cout << s << " ";
    }
    cout << endl << endl;

    cout << "Depósito inicial: " << initial_depot << endl;
    cout << "Tempo de operação da linha: " << max_time << " (" << max_time / 3600 << " horas)" << endl << endl;

    for (int i = 0; i < time_intervals.size(); i++)
    {
        cout << "Intervalo " << i << ": (" << get <0> (time_intervals[i]) << ", " << get <1> (time_intervals[i]) << ")" << endl;
    }
    cout << endl;

    // header with indexes of columns
    cout << "     ";
    for (int i = 0; i < points.size()*2; i++) {
        cout << setw(5) << i;
    }
    cout << "\n";
    // matrix wtih indexes of rows
    for (int i = 0; i < points.size()*2; i++) {
        cout << setw(3) << i << "   ";
        for (int j = 0; j < points.size()*2; j++) {
            cout << setw(5) << adj_matrix[i][j];
        }
        cout << "\n";
    }

    cout << endl << "number of routes = " << routes.size() << endl;      
    cout << "vertices of routes..." << endl;
    for (int i = 0; i < routes.size(); i++)
    {
        cout << "\troute #" << i << ": ";
        vector <int> route_vertices = routes[i];
        for (int j = 0; j < route_vertices.size(); j++)
        {
            cout << route_vertices[j] << setw(6);
        }
        cout << endl;
    }

    // display demands of time intervals
    cout << "demands..." << endl;
    for (int i = 0; i < points.size()*2; i++)
    {
        cout << "\tdemands of vertex " << i << " at time interval: ";
        for (int j = 0; j < time_intervals.size(); j++)
        {
            cout << demands[i][j] << setw(6);
        }
        cout << endl;
    }
    cout << endl;

    // display minimum service
    cout << "minimum service time: " << endl;
    for (int i = 0; i < service_time_min.size(); i++)
    {
        cout << service_time_min[i] << " ";
    }
    cout << endl;

    // display maximum service
    cout << "maximum service time: " << service_time_max[0] << endl;;

    // display alpha
    cout << "alpha = " << headway << endl; 
}

void write_instance (int nb_trains, vector <int> &max_nb_trips_per_train, vector <int> &points, vector <int> &depots, vector <int> &crossings, vector <int> &stations, int initial_depot, int max_time, vector <tuple<int, int>> &time_intervals, vector<vector<int>> &adj_matrix, vector<vector <int>> &routes, vector <vector <int>> &demands, vector <int> &service_time_min, vector <int> &service_time_max, int headway)
{
    ofstream file("ex-instance.txt");
 
    // writing instance on file
    file << "#num_trains" << endl;
    file << nb_trains << endl << endl;

    file << "#num_trips" << endl;
    for (auto i : max_nb_trips_per_train)
        file << i << "   ";
    file << endl << endl;

    file << "#num_intervals" << endl;
    file << time_intervals.size() << endl << endl;

    file << "#time_intervals" << endl;
    for (auto i : time_intervals)
        file << get <0> (i) << "     " << get <1> (i) << endl ;
    file << endl;

    file << "#num_points" << endl;
    file << points.size() << endl << endl;

    file << "#num_stations" << endl;
    file << stations.size() << endl << endl;
    file << "#stations" << endl;
    for (auto s : stations)
        file << s << "   ";
    file << endl << endl;

    file << "#num_crossings" << endl;
    file << crossings.size() << endl << endl;
    file << "#crossings" << endl;
    for (auto c : crossings)
        file << c << "   ";
    file << endl << endl;

    file << "#num_depots" << endl;
    file << depots.size() << endl << endl;
    file << "#depots" << endl;
    for (auto d : depots)
        file << d << "   ";
    file << endl << endl;

    file << "#initial_point" << endl;
    file << initial_depot << endl << endl;

    file << "#num_routes" << endl;
    file << routes.size() << endl << endl;

    file << "#routes" << endl;
    for (auto r : routes)
    {
        for (auto v : r)
        {
            file << v << "   ";
        }

        if(r.size() < points.size()*2)
        {
            for (int i = r.size(); i <= points.size()*2; i++)
            {
                file << "-1   ";
            }
        }
        file << endl;
    }
    file << endl;

    file << "#service_time_min" << endl;
    for (auto s : service_time_min)
        file << s << "   ";
    file << endl << endl;

    file << "#service_time_max" << endl;
    for (auto s : service_time_max)
        file << s << "   ";
    file << endl << endl;

    // matriz
    file << "#cost_matrix" << endl;
    // header with indexes of columns
    for (int i = 0; i < points.size()*2; i++) {
        file << "    " << i;
    }
    file << endl;
    // matrix with indexes of rows
    for (int i = 0; i < points.size()*2; i++) {
        file << i;
        for (int j = 0; j < points.size()*2; j++) {
            file << "   " << adj_matrix[i][j];
        }
        file << endl;
    }
    file << endl;


    file << "#demands" << endl;
    for (int i = 0; i < points.size()*2; i++)
    {
        for (int j = 0; j < time_intervals.size(); j++)
        {
            file << demands[i][j] << " ";
        }
        file << endl;
    }
    file << endl;
    

    file << "#max_time" << endl;
    file << max_time << endl << endl;

    file << "#alpha" << endl;
    file << headway << endl << endl;

    file.close();
}

void generate_instance (int p, int t)
{
    srand(chrono::high_resolution_clock::now().time_since_epoch().count());

    // int nb_points = random_nb_points();
    int nb_points = p;

    // generate list with all the points
    vector <int> points;
    vector <int> depots, crossings, stations;
    for (int i = 0; i < nb_points; i++)
    {
        points.push_back(i);
    }

    generate_network(points, depots, crossings, stations);
    int initial_depot = random_initial_depot(depots);

    // int nb_trains = random_nb_trains(nb_points);
    int nb_trains = t;

    int max_time = random_max_time();
    vector<vector<int>> adj_matrix (nb_points*2, vector<int>(nb_points*2, -1));
    generate_arcs(max_time, nb_points, depots, adj_matrix);
    vector <tuple <int, int>> time_intervals;
    generate_time_intervals(max_time, time_intervals);

    vector <vector <int>> routes;
    generate_routes(routes, depots, nb_points);

    vector <int> service_time_min (nb_points);
    vector <int> service_time_max (nb_points);
    int headway;
    generate_service_and_headway(max_time, points, depots, crossings, stations, service_time_min, service_time_max, headway);
    
    vector <vector <int>> demands (nb_points*2, vector<int>(time_intervals.size(), 0));
    vector <int> max_nb_trips_per_train (nb_trains);
    generate_demands_and_nb_trips(demands, max_nb_trips_per_train, depots, crossings, stations, time_intervals.size(), nb_points, max_time);

    // print_instance(nb_trains, max_nb_trips_per_train, points, depots, crossings, stations, initial_depot, max_time, time_intervals, adj_matrix, routes, demands, service_time_min, service_time_max, headway);
    write_instance(nb_trains, max_nb_trips_per_train, points, depots, crossings, stations, initial_depot, max_time, time_intervals, adj_matrix, routes, demands, service_time_min, service_time_max, headway);

}

// string create_name(const string& base, const string& extensao, const string& pasta) {
//     string nome_completo = pasta + "/" + base + extensao;

//     if (!filesystem::exists(nome_completo))
//         return nome_completo;

//     // tenta com sufixos A, B, C, ..., Z
//     for (char letra = 'A'; letra <= 'Z'; letra++)
//     {
//         ostringstream nome;
//         nome << pasta << "/" << base << letra << extensao;
//         if (!filesystem::exists(nome.str()))
//             return nome.str();
//     }

//     throw runtime_error("Todos os sufixos de A a Z já existem!");
// }

// Usage: ./random-generator <set-name> <threads>

// Create instances according to size: number of points and number of trains
int main(int argc, char *argv[])
{
    set_name = argv[1]; // 5-to-9, 10-to-14, 15-to-19, 20-to-24...

    pair <int, int> trains_range;
    pair <int, int> points_range;

    if (set_name == "5-to-9")
    {
        trains_range = {2, 5};
        points_range = {5, 9};
    }
    else if (set_name == "10-to-14")
    {
        trains_range = {3, 6};
        points_range = {10, 14};
    }
    else if (set_name == "15-to-19")
    {
        trains_range = {3, 6};
        points_range = {15, 19};
    }
    else if (set_name == "20-to-24")
    {
        trains_range = {3, 6};  
        points_range = {20, 24};
    }

    for (int p = points_range.first; p <= points_range.second; p++)
    {
        for (int t = trains_range.first; t <= trains_range.second; t++)
        {
            // if instance with points = p and trains = t already exists, skip
            bool instance_exists = false;
            string pattern = "t" + to_string(t) + "p" + to_string(p);
            for (const auto& entry : filesystem::directory_iterator("../experiments/" + set_name)) {
                if (entry.is_regular_file()) {
                    string filename = entry.path().filename().string();
                    if (filename.substr(0, pattern.length()) == pattern) {
                        instance_exists = true;
                        break;
                    }
                }
            }
            if (instance_exists)
                continue;

            cout << "Generating instance with " << p << " points and " << t << " trains..." << endl;

            bool not_feasible = true;
            while (not_feasible)
            {
                generate_instance(p, t);
                Data data("ex-instance.txt");
                data.print_data();

                // // uses model to verify if instance is feasible
                // Model model;
                // model.verify_feasibility = true;
                // model.initialize(data);  
                // bool solved = model.run(data);

                // uses enumeration to verify if instance is feasible
                Combinations comb(data, stoi(argv[2]), 0, 3600, true);
                bool solved = comb.nb_feasible_combinations > 0;

                if (solved)
                {
                    // save instance on file
                    string origem = "ex-instance.txt";
                    ostringstream nome_destino;
                    nome_destino << "t" << data.get_nb_trains() << "p" << data.get_nb_points()
                                 << "r" << data.get_nb_routes() <<  "h" << data.get_nb_intervals();
                    
                    // create directory if it doesn't exist
                    string dir_path = "../experiments/" + set_name;
                    if (!filesystem::exists(dir_path))
                        filesystem::create_directories(dir_path);
                    string destino = dir_path + "/" + nome_destino.str() + ".txt";
                            
                    try
                    {
                        not_feasible = false;
                        filesystem::copy(origem, destino, filesystem::copy_options::overwrite_existing);
                        cout << "Instância criada com sucesso! - " << p << ", " << t << endl;
                    }
                    catch (filesystem::filesystem_error& e)
                    {
                        cerr << "Erro ao copiar: " << e.what() << endl;
                    }
                }
            }
        }    
    }

    return 0;
}