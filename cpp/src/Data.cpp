#include "Data.hpp"

using namespace std;

Data::Data(string instance_path)
{
    _instance_path = instance_path;

    // extracting the name of the instance (without path and .txt)
    std::string separate = "/";
    std::stringstream ss(_instance_path);
    std::string instance_full;
    if (std::getline(ss, _instance_name, separate[0]))
    {
        while (std::getline(ss, instance_full, separate[0]))
        {
            _instance_name = instance_full;
        }
    }
    if (_instance_name.size() > 4 && _instance_name.rfind(".txt") == _instance_name.size() - 4)
    {
        _instance_name.erase(_instance_name.size() - 4);
    }

    // extracting the set of the instance (folder name)
    std::stringstream ss_set(_instance_path);
    std::string token;
    std::vector<std::string> path_parts;
    while (std::getline(ss_set, token, '/'))
    {
        path_parts.push_back(token);
    }
    // path format: Train-Timetabling/instances/<set>/<instance>.txt
    if (path_parts.size() >= 2)
    {
        _instance_set = path_parts[path_parts.size() - 2];
    }
    else
    {
        _instance_set = "unknown";
    }

    // try to open the instance file
    cout << "   > Trying to open the instance file..." << endl;
    ifstream instance_file;
    instance_file.open(instance_path.c_str());
    if (!instance_file)
    {
        cerr << "## Error: Could not open instance file (at Instance::load, line " << __LINE__ << ")." << endl;
        cout << "   > Process terminated!" << endl;
        exit(0);
    }

    // read #_nb_trains
    cout << "   > Reading number of trains..." << endl;
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    instance_file >> _nb_trains;

    // read #_max_trips_per_train
    cout << "   > Reading maximum number of trips per train..." << endl;
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    _max_trips_per_train.assign(_nb_trains, 0);
    _max_nb_trips = 0;
    for (int i = 0; i < _nb_trains; i++)
    {
        instance_file >> _max_trips_per_train[i];
        if (_max_trips_per_train[i] > _max_nb_trips)
            _max_nb_trips = _max_trips_per_train[i];
    }

    // read #_time_intervals
    cout << "   > Reading time intervals..." << endl;
    int nb_intervals;
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    instance_file >> nb_intervals;
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    for (int i = 0; i < nb_intervals; i++)
    {
        pair<int, int> aux_interval;
        instance_file >> get<0>(aux_interval);
        instance_file >> get<1>(aux_interval);
        _time_intervals.push_back(aux_interval);
    }

    // read #_nb_points
    cout << "   > Reading number of points..." << endl;
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    instance_file >> _nb_points;

    // assign _upper_vertex_set and _lower_vertex_set
    _point_to_vertices.assign(_nb_points, vector<int>(2, 0));
    for (int i = 0; i < _nb_points; i++)
    {
        _upper_vertex_set.push_back(i);
        _vertex_to_point.push_back(i);
        _point_to_vertices[i][0] = i;
    }
    for (int i = _nb_points; i < _nb_points * 2; i++)
    {
        _lower_vertex_set.push_back(i);
        _vertex_to_point.push_back(i - _nb_points);
        _point_to_vertices[i - _nb_points][1] = i;
    }

    // assign _is_station
    cout << "   > Reading stations..." << endl;
    int nb_stations, station;
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    instance_file >> nb_stations;
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    _is_station.assign(_nb_points, false);
    for (int i = 0; i < nb_stations; i++)
    {
        instance_file >> station;
        _is_station[station] = true;
    }

    // assign _is_crossing
    cout << "   > Reading crossings..." << endl;
    int nb_crossings, crossing;
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    instance_file >> nb_crossings;
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    _is_crossing.assign(_nb_points, false);
    for (int i = 0; i < nb_crossings; i++)
    {
        instance_file >> crossing;
        _is_crossing[crossing] = true;
    }

    // assign _is_depot
    cout << "   > Reading depots..." << endl;
    int nb_depots, depot;
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    instance_file >> nb_depots;
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    _is_depot.assign(_nb_points, false);
    for (int i = 0; i < nb_depots; i++)
    {
        instance_file >> depot;
        _is_depot[depot] = true;
    }

    // read #_initial_point
    cout << "   > Reading initial point..." << endl;
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    instance_file >> _initial_point;

    // verify whether instance is consistent
    if (!validate_instance())
        exit(0);

    // read routes
    cout << "   > Reading routes..." << endl;
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    instance_file >> _nb_routes;
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    for (int i = 0; i < _nb_routes; i++)
    {
        vector<int> route_verts;
        int vertex;
        for (int j = 0; j < get_nb_vertices()+1; j++)
        {
            instance_file >> vertex;
            if (vertex != -1)
            {
                route_verts.push_back(vertex);
            }
        }
        _route_vertices.push_back(route_verts);
    }

    // assign #_arcs
    assign_arcs();

    // read #_distance, #_distance_and_service_min, #_distance_and_service_max
    cout << "   > Reading distance and distance and service..." << endl;
    // reading min service time
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    int aux_service_time_min[get_nb_vertices()];
    for (int i = 0; i < _nb_points; i++)
    {
        instance_file >> aux_service_time_min[i];
        aux_service_time_min[i + get_nb_points()] = aux_service_time_min[i];
    }
    // reading max service time
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    int aux_service_time_max[get_nb_vertices()];
    for (int i = 0; i < _nb_points; i++)
    {
        instance_file >> aux_service_time_max[i];
        aux_service_time_max[i + get_nb_points()] = aux_service_time_max[i];
    }
    // reading distances
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    instance_file.ignore(100000, '\n');
    int aux_ignore = 0;
    int distance_aux[get_nb_vertices()][get_nb_vertices()];
    int distance_and_service_min_aux[get_nb_vertices()][get_nb_vertices()];
    int distance_and_service_max_aux[get_nb_vertices()][get_nb_vertices()];
    for (int i = 0; i < get_nb_vertices(); i++)
    {
        instance_file >> aux_ignore;
        for (int j = 0; j < get_nb_vertices(); j++)
        {
            instance_file >> distance_aux[i][j];
            distance_and_service_min_aux[i][j] = distance_aux[i][j];
            distance_and_service_max_aux[i][j] = distance_aux[i][j];
            Arc arc;
            arc.out = i;
            arc.inc = j;
            if (!is_reversal_arc(arc))
                distance_and_service_min_aux[i][j] += aux_service_time_min[j];
                distance_and_service_max_aux[i][j] += aux_service_time_max[j];
                
        }
    }
    _distance.assign(get_nb_arcs(), 0);
    _distance_and_service_min.assign(get_nb_arcs(), 0);
    _distance_and_service_max.assign(get_nb_arcs(), 0);
    for (int a = 0; a < get_nb_arcs(); a++)
    {
        Arc arc = _arcs[a];
        _distance[a] = distance_aux[arc.out][arc.inc];
        _distance_and_service_min[a] = distance_and_service_min_aux[arc.out][arc.inc];
        _distance_and_service_max[a] = distance_and_service_max_aux[arc.out][arc.inc];
    }

    // read #_demands
    cout << "   > Reading demands..." << endl;
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    _demands.assign(get_nb_vertices(), vector<int>(get_nb_intervals(), 0));
    for (int i = 0; i < get_nb_vertices(); i++)
    {
        for (int j = 0; j < get_nb_intervals(); j++)
        {
            instance_file >> _demands[i][j];
        }
    }
    // get demands of vertices for a whole day
    _demand_per_day.assign(get_nb_vertices(), 0);
    for (int i = 0; i < get_nb_vertices(); i++)
    {
        for (int j = 0; j < get_nb_intervals(); j++)
            _demand_per_day[i] += get_demands()[i][j];
    }

    // read #_max_time
    cout << "   > Reading maximum time..." << endl;
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    instance_file >> _max_time;

    // read #_alpha
    cout << "   > Reading alpha..." << endl;
    instance_file.ignore(100000, '#');
    instance_file.ignore(100000, '\n');
    instance_file >> _alpha;

    // assign #_incompatible_points and create routes
    assign_incompatible_points();
    // create_routes(); // method not being called (routes are now an input to the instance)
}

// method that creates all possible routes for the train line, according to the depots
void Data::create_routes()
{
    // create a cyclic route for depots located at the middle of the train line
    vector<int> route_verts;
    for (int i = 0; i < _nb_points; i++)
    {
        int last_point = _nb_points-1;
        if (_is_depot[i] && (i != 0 && i != last_point))
        {
            // cyclic starting from upper vertices
            for (int vertex = i; vertex <= last_point; vertex++)
                route_verts.push_back(vertex);
            for (int point = last_point; point >= 0; point--)
            {
                int vertex = point + _nb_points;
                route_verts.push_back(vertex);
            }
            for (int vertex = 0; vertex <= i; vertex++)
                route_verts.push_back(vertex);
            _route_vertices.push_back(route_verts);

            // cyclic starting from lower vertices
            route_verts.clear();
            for (int point = i; point >= 0; point--)
            {
                int vertex = point + _nb_points;
                route_verts.push_back(vertex);
            }
            for (int vertex = 0; vertex <= last_point; vertex++)
                route_verts.push_back(vertex);
            for (int point = last_point; point >= i; point--)
            {
                int vertex = point + _nb_points;
                route_verts.push_back(vertex);
            }
            _route_vertices.push_back(route_verts);
        }
    }

    vector<pair<int, int>> depot_pairs;
    for (int i = 0; i < _nb_points; i++)
    {
        for (int j = i + 1; j < _nb_points; j++)
        {
            if (_is_depot[i] && _is_depot[j])
            {
                depot_pairs.push_back(make_pair(i, j));
            }
        }
    }

    // for each depot pair, we build 4 routes
    for (int i = 0; i < depot_pairs.size(); i++)
    {
        int first_point = depot_pairs[i].first;
        int second_point = depot_pairs[i].second;

        // from first to second, no cycle.
        // the route_verts contains only arcs in the upper section
        route_verts.clear();
        for (int vertex = first_point; vertex <= second_point; vertex++)
            route_verts.push_back(vertex);
        _route_vertices.push_back(route_verts);

        // from first to second, with cycle.
        // the route_verts contains upper arcs from first to second, a reversal arc and
        // lower arcs from second to first
        route_verts.clear();
        for (int vertex = first_point; vertex <= second_point; vertex++)
            route_verts.push_back(vertex);
        for (int point = second_point; point >= first_point; point--)
        {
            int vertex = point + _nb_points;
            route_verts.push_back(vertex);
        }
        route_verts.push_back(first_point);
        _route_vertices.push_back(route_verts);

        // from second to first, no cycle.
        // the route_verts contains only arcs in the lower section
        route_verts.clear();
        for (int point = second_point; point >= first_point; point--)
        {
            int vertex = point + _nb_points;
            route_verts.push_back(vertex);
        }
        _route_vertices.push_back(route_verts);

        // from second to first, with cycle.
        // the route_verts contains lower arcs from second to first, a reversal arc and
        // upper arcs from first to second
        route_verts.clear();
        for (int point = second_point; point >= first_point; point--)
        {
            int vertex = point + _nb_points;
            route_verts.push_back(vertex);
        }
        for (int vertex = first_point; vertex <= second_point; vertex++)
            route_verts.push_back(vertex);
        route_verts.push_back(second_point + _nb_points);
        _route_vertices.push_back(route_verts);
    }

    _nb_routes = _route_vertices.size();

    // for each route, we build the arcs
    for (int i = 0; i < _route_vertices.size(); i++)
    {
        vector<int> &route_verts = _route_vertices[i];
        vector<Arc> route_arcs;
        for (int j = 0; j < route_verts.size() - 1; j++)
        {
            // find arc that belongs to the route
            for (int a = 0; a < _arcs.size(); a++)
            {
                if (_arcs[a].out == route_verts[j] && _arcs[a].inc == route_verts[j + 1])
                {
                    route_arcs.push_back(_arcs[a]);
                    break;
                }
            }
        }
        _route_arcs.push_back(route_arcs);
    }
}

void Data::assign_incompatible_points()
{
    // for each upper vertex that is a crossing
    for (int i = 0; i < _nb_points - 1; i++)
    {
        if (_is_crossing[_vertex_to_point[i]])
        {
            // find the next crossing in the lower section
            int next_crossing = -1;
            for (int j = i + _nb_points + 1; j < 2 * _nb_points; j++)
            {
                if (_is_crossing[_vertex_to_point[j]])
                {
                    next_crossing = j;
                    break;
                }
            }
            assert(next_crossing != -1);

            // considering that the train in the upper section departs first
            int k = i;
            int v = next_crossing;
            int q = next_crossing - _nb_points - 1;
            Arc a;
            for (int a_idx = 0; a_idx < _arcs.size(); a_idx++)
            {
                // find arc a = (q, q + 1)
                if (_arcs[a_idx].out == q && _arcs[a_idx].inc == q + 1)
                {
                    a = _arcs[a_idx];
                    break;
                }
            }
            _inc_points.push_back(make_tuple(k, q, v, a));

            // considering that the train in the lower section departs first
            k = next_crossing;
            v = i;
            q = i + _nb_points + 1;
            for (int a_idx = 0; a_idx < _arcs.size(); a_idx++)
            {
                // find arc a = (q, q - 1)
                if (_arcs[a_idx].out == q && _arcs[a_idx].inc == q - 1)
                {
                    a = _arcs[a_idx];
                    break;
                }
            }
            _inc_points.push_back(make_tuple(k, q, v, a));
        }
    }
}

void Data::assign_arcs()
{
    for (int i = 0; i < _nb_points; i++)
    {
        if (i < _nb_points - 1)
        {
            Arc upper_arc;
            upper_arc.out = i;
            upper_arc.inc = i + 1;
            upper_arc.idx = _arcs.size();
            _arcs.push_back(upper_arc);
        }

        if (i > 0)
        {
            Arc lower_arc;
            lower_arc.out = i + _nb_points;
            lower_arc.inc = i + _nb_points - 1;
            lower_arc.idx = _arcs.size();
            _arcs.push_back(lower_arc);
        }

        if (_is_depot[i])
        {
            Arc upper_lower_arc;
            upper_lower_arc.out = i;
            upper_lower_arc.inc = i + _nb_points;
            upper_lower_arc.idx = _arcs.size();
            _arcs.push_back(upper_lower_arc);

            Arc lower_upper_arc;
            lower_upper_arc.out = i + _nb_points;
            lower_upper_arc.inc = i;
            lower_upper_arc.idx = _arcs.size();
            _arcs.push_back(lower_upper_arc);
        }
    }

    _vertex_out_arcs.assign(get_nb_vertices(), {});
    _vertex_inc_arcs.assign(get_nb_vertices(), {});
    for (int v = 0; v < get_nb_vertices(); v++)
    {
        for (Arc &arc : _arcs)
        {
            if (arc.out == v)
            {
                _vertex_out_arcs[v].push_back(arc);
            }
            if (arc.inc == v)
            {
                _vertex_inc_arcs[v].push_back(arc);
            }
        }
    }

    // for each route, we build the arcs
    for (int i = 0; i < _route_vertices.size(); i++)
    {
        vector<int> &route_verts = _route_vertices[i];
        vector<Arc> route_arcs;
        for (int j = 0; j < route_verts.size() - 1; j++)
        {
            // find arc that belongs to the route
            for (int a = 0; a < _arcs.size(); a++)
            {
                if (_arcs[a].out == route_verts[j] && _arcs[a].inc == route_verts[j + 1])
                {
                    route_arcs.push_back(_arcs[a]);
                    break;
                }
            }
        }
        _route_arcs.push_back(route_arcs);
    }
}

bool Data::validate_instance()
{
    for (int i = 0; i < _nb_points; i++)
    {
        // check if all depots also are stations and crossings
        if ((_is_depot[i]) && !(_is_station[i] && _is_crossing[i]))
        {
            cout << "Error: Instance is inconsistent!" << endl;
            cout << "\t>> All depots must be also classified as stations and crossings" << endl;
            return false;
        }    
        // check if initial point is a depot
        if ((_nb_points == _initial_point) && !(_is_depot[i]))
        {
            cout << "Error: Instance is inconsistent!" << endl;
            cout << "\t>> The initial point must be a depot" << endl;
            return false;
        }
    }

    return true;
}

void Data::print_data()
{
    cout << endl
         << "--------------------------------\n"
         << " Printing instance\n"
         << "--------------------------------\n";

    // display number of trains and trips
    cout << "number of trains = " << _nb_trains << endl;
    cout << "\tmax trips per trains: ";
    for(int i = 0; i < _max_trips_per_train.size(); i++)
        cout << _max_trips_per_train[i] << setw(6);
    cout << endl;

    // display time intervals
    cout << "number of intervals = " << get_nb_intervals() << endl;
    for (int i = 0; i < get_nb_intervals(); i++)
        cout << "\t(" << _time_intervals[i].first << ", " << setw(6) << _time_intervals[i].second << ")";
    cout << endl;

    // display points, stations, crossings and depots
    cout << "number of points = " << _nb_points << endl;
    cout << "\tinitial point: " << _initial_point << endl;
    cout << "\tstations: ";  
    for (int i = 0; i < _nb_points; i++)
        if(_is_station[i])
            cout << i << setw(6);
    cout << endl << "\tcrossings: ";
    for (int i = 0; i < _nb_points; i++)
        if(_is_crossing[i])
            cout << i << setw(6);
    cout << endl << "\tdepots: ";  
    for (int i = 0; i < _nb_points; i++)
        if(_is_depot[i])
            cout << i << setw(6);
    cout << endl;

    // display sets of vertices
    cout << "number of vertices = " << get_nb_vertices() << endl;
    cout << "\ttotal vertices: ";
    for (int i = 0; i < get_nb_vertices(); i++)
        cout << i << setw(6);
    cout << endl;
    cout << "\tupper vertices: ";
    for (int i = 0; i < _upper_vertex_set.size(); i++)
        cout << _upper_vertex_set[i] << setw(6);
    cout << endl;
    cout << "\tlower vertices: ";
    for (int i = 0; i < _lower_vertex_set.size(); i++)
        cout << _lower_vertex_set[i] << setw(6);
    cout << endl;
    for (int i = 0; i < _nb_points; i++)
    {
        cout << "\tvertices of point #" << i << ": ";
        cout << "(" << _point_to_vertices[i][0] << ", " << _point_to_vertices[i][1] << ")" << setw(6);
        cout << endl;
    }

    // display sets of arcs
    cout << endl << "arcs..." << endl;
    cout << "\ttotal arcs: " << endl;
    for (int i = 0 ; i < _arcs.size(); i++)                    
        cout << "\t\tarc #" << _arcs[i].idx << ": (" << _arcs[i].out << ", " << _arcs[i].inc << ")" << endl;
    cout << endl;
    cout << "\treverse arcs: " << endl;
    for (int i = 0 ; i < _arcs.size(); i++)
    {
        if (is_reversal_arc(_arcs[i]))
        {
            cout << "\t\tarc #" << _arcs[i].idx << ": (" << _arcs[i].out << ", " << _arcs[i].inc << ")" << endl;   
        }
    }                 
    cout << endl;

    // display set of incompatible points
    cout << endl << "incompatible points..." << endl;
    for (int i = 0; i < _inc_points.size(); i++)
    {
        int k = get <0> (_inc_points[i]);
        int q = get <1> (_inc_points[i]);
        int v = get <2> (_inc_points[i]);
        Arc a = get <3> (_inc_points[i]);
        cout << "\t(" << k << ", " << q << ", " << v << ", (" << a.out << ", " << a.inc << "))";
    }
    cout << endl;

    // display sets of routes
    cout << endl << "number of routes = " << get_nb_routes() << endl;      
    cout << "vertices of routes..." << endl;
    for (int i = 0; i < get_nb_routes(); i++)
    {
        cout << "\troute #" << i << ": ";
        for (int j = 0; j < _route_vertices[i].size(); j++)
        {
            cout << _route_vertices[i][j] << setw(6);
        }
        cout << endl;
    }
    cout << "arcs of routes..." << endl;
    for (int i = 0; i < get_nb_routes(); i++)
    {
        cout << "\troute #" << i << ": ";
        for (int j = 0; j < _route_arcs[i].size(); j++)
        {
            cout << _route_arcs[i][j].idx << setw(6);
        }
        cout << endl;
    }

    // display pairs of incompatible routes
    cout << endl << "incompatible routes..." << endl;
    for (int i = 0; i < get_nb_routes(); i++)
    {  
        for (int j = 0; j < get_nb_routes(); j++)
        {
            if(are_incompatible_routes(i, j))
            {
                cout << "\t(" << i << ", " << j << ")" << setw(6);
            }
        }
    }
    cout << endl;

    // display vertices that can be adjacent in consecutive trips
    cout << endl << "vertices that can be adjacent in consecutive trips..." << endl;
    for (int i = 0; i < get_nb_vertices(); i++)
    {
        for (int j = 0; j < get_nb_vertices(); j++)
        {
            if (can_be_adjacent_in_consecutive_trips(i, j))
            {
                cout << "\t(" << i << ", " << j << ")" << setw(6);
            }
        }  
    }
    cout << endl;

    // display non cyclical routes
    cout << "non cyclical routes: " << endl;
    for (int i = 0; i < get_nb_routes(); i++)
    {
        if (!is_cyclic_route(i))
        {
            cout << setw(8) << i << setw(8);
        }
    }
    cout << endl;

    // display routes that trains can complete in each trip
    cout << endl;
    cout << "routes of trip..." << endl;
    for (int i = 0; i < _nb_trains; i++)
    {
        for (int j = 0; j < _max_trips_per_train[i]; j++)
        {
            cout << "\troutes that train #" << i << " can complete at trip #" << j << ": ";
            for (int k = 0; k < get_nb_routes(); k++)
            {
                if (is_valid_route(i, j, k))
                {
                    cout << k << setw(8);
                }
            }
            cout << endl;
        }
        cout << endl;
    }

    // display distances
    cout << "cost..." << endl << setprecision(0) << fixed;
    for (int i  = 0; i < _arcs.size(); i++)
    {
        cout << "\tarc #" << _arcs[i].idx << " [" << _arcs[i].out << "][" << _arcs[i].inc << "]: " << _distance[i] << endl;
    }
    cout << endl;

    // display distance with minimum service cost
    cout << "cost with minimum service..." << endl << setprecision(0) << fixed;
    for (int i  = 0; i < _arcs.size(); i++)
    {
        cout << "\tarc #" << _arcs[i].idx << " [" << _arcs[i].out << "][" << _arcs[i].inc << "]: " << _distance_and_service_min[i] << endl;
    }
    cout << endl;

    // display distance with maximum service cost
    cout << "cost with maximum service..." << endl << setprecision(0) << fixed;
    for (int i  = 0; i < _arcs.size(); i++)
    {
        cout << "\tarc #" << _arcs[i].idx << " [" << _arcs[i].out << "][" << _arcs[i].inc << "]: " << _distance_and_service_max[i] << endl;
    }
    cout << endl;

    // display demands of time intervals
    cout << "demands..." << endl;
    for (int i = 0; i < get_nb_vertices(); i++)
    {
        cout << "\tdemands of vertex " << i << " at time interval: ";
        for (int j = 0; j < get_nb_intervals(); j++)
        {
            cout << _demands[i][j] << setw(6);
        }
        cout << endl;
    }
    cout << endl;

    // display maximum time
    cout << "maximum time = " << _max_time << endl;

    // display alpha
    cout << "alpha = " << _alpha << endl; 
}