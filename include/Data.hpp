#ifndef DATA_HPP
#define DATA_HPP

#include <vector>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cassert>

struct Arc
{
    int out;
    int inc;
    int idx;
};

typedef std::tuple<int, int, int, Arc> IncompatiblePoints;

class Data
{
private:
    std::string _instance_path;
    std::string _instance_name;

    int _nb_trains;
    int _nb_points;
    int _initial_point;
    int _nb_routes;
    // one entry for each point
    std::vector<bool> _is_station;
    std::vector<bool> _is_crossing;
    std::vector<bool> _is_depot;
    // vertices numbered from 0 to nb_points-1
    std::vector<int> _upper_vertex_set;
    // vertices numbered from nb_points to 2*nb_points-1
    std::vector<int> _lower_vertex_set;
    // the vertices associated with each point
    std::vector<std::vector<int>> _point_to_vertices;
    // the point associated with each vertex
    std::vector<int> _vertex_to_point;
    // the maximum number of trips per train
    std::vector<int> _max_trips_per_train;
    // time intervals
    std::vector<std::pair<int, int>> _time_intervals;
    // all arcs
    std::vector<Arc> _arcs;
    // arcs leaving each vertex
    std::vector<std::vector<Arc>> _vertex_out_arcs;
    // arcs entering each vertex
    std::vector<std::vector<Arc>> _vertex_inc_arcs;
    // arcs of each route
    std::vector<std::vector<Arc>> _route_arcs;
    // vertices of each route
    std::vector<std::vector<int>> _route_vertices;
    // incompatible points
    std::vector<IncompatiblePoints> _inc_points;
    // distance values
    std::vector<int> _distance;
    // distance and min service values
    std::vector<int> _distance_and_service_min;
    // distance and max service values
    std::vector<int> _distance_and_service_max;
    // demands of each vertex by time intervals
    std::vector<std::vector<int>> _demands;
    // maximum time for all of the trips to end
    int _max_time;
    // alpha - headway time
    int _alpha;

public:
    Data(std::string instance_path);

    void print_data();

    void create_routes();

    void assign_incompatible_points();

    void assign_arcs();

    std::string get_instance_path()
    {
        return _instance_path;
    }

    std::string get_instance_name()
    {
        return _instance_name;
    }

    int get_nb_trains()
    {
        return _nb_trains;
    }

    int get_nb_points()
    {
        return _nb_points;
    }

    int get_nb_vertices()
    {
        return 2 * _nb_points;
    }

    int get_nb_routes()
    {
        return _nb_routes;
    }

    int get_nb_arcs()
    {
        return _arcs.size();
    }

    std::vector<Arc> &get_arcs()
    {
        return _arcs;
    }

    std::vector<Arc> &get_vertex_inc_arcs(int vertex)
    {
        return _vertex_inc_arcs[vertex];
    }

    std::vector<Arc> &get_vertex_out_arcs(int vertex)
    {
        return _vertex_out_arcs[vertex];
    }

    std::vector<Arc> &get_route_arcs(int route_idx)
    {
        return _route_arcs[route_idx];
    }

    std::vector<int> &get_route_vertices(int route_idx)
    {
        return _route_vertices[route_idx];
    }

    bool is_reversal_arc(Arc arc)
    {
        return std::abs(arc.out - arc.inc) == _nb_points;
    }

    bool is_station(int point)
    {
        return _is_station[point];
    }

    std::vector<int> &get_upper_vertex_set()
    {
        return _upper_vertex_set;
    }

    std::vector<int> &get_lower_vertex_set()
    {
        return _lower_vertex_set;
    }

    std::vector<int> &get_point_vertices(int point)
    {
        return _point_to_vertices[point];
    }

    std::vector<std::vector<int>> &get_demands()
    {
        return _demands;
    }

    int get_vertex_point(int vertex)
    {
        return _vertex_to_point[vertex];
    }

    int get_train_max_trips(int train)
    {
        return _max_trips_per_train[train];
    }

    int get_nb_intervals()
    {
        return _time_intervals.size();
    }

    int get_interval_start(int interval_idx)
    {
        return _time_intervals[interval_idx].first;
    }

    int get_interval_end(int interval_idx)
    {
        return _time_intervals[interval_idx].second;
    }

    std::vector<IncompatiblePoints> &get_inc_points()
    {
        return _inc_points;
    }

    int get_distance(int arc_idx)
    {
        return _distance[arc_idx];
    }

    int get_distance_and_service_min(int arc_idx)
    {
        return _distance_and_service_min[arc_idx];
    }

    int get_distance_and_service_max(int arc_idx)
    {
        return _distance_and_service_max[arc_idx];
    }

    int get_max_time()
    {
        return _max_time;
    }

    int get_alpha()
    {
        return _alpha;
    }

    bool is_valid_route(int train_idx, int trip_idx, int route_idx)
    {
        // train must start the day at the initial depot
        std::vector<Arc> &route_arcs = get_route_arcs(route_idx);
        if (trip_idx == 0)
        {
            return get_vertex_point(route_arcs[0].out) == _initial_point;
        }
        return true;
    }

    bool start_at_vertex(int route_idx, int vertex)
    {
        std::vector<int> &route_vertices = get_route_vertices(route_idx);
        return route_vertices[0] == vertex;
    }

    bool finish_at_vertex(int route_idx, int vertex)
    {
        std::vector<int> &route_vertices = get_route_vertices(route_idx);
        return route_vertices[route_vertices.size() - 1] == vertex;
    }

    bool finish_at_initial_point(int route_idx)
    {
        return finish_at_vertex(route_idx, _initial_point) || finish_at_vertex(route_idx, _initial_point + _nb_points);
    }

    bool are_incompatible_routes(int route1_idx, int route2_idx)
    {
        int route1_last_vertex = get_route_vertices(route1_idx)[get_route_vertices(route1_idx).size() - 1];
        int route2_first_vertex = get_route_vertices(route2_idx)[0];
        return get_vertex_point(route1_last_vertex) != get_vertex_point(route2_first_vertex);
    }

    bool can_be_adjacent_in_consecutive_trips(int vertex1, int vertex2)
    {
        int nb_routes_ending_at_v1 = 0;
        int nb_routes_starting_at_v2 = 0;
        for (int route_idx = 0; route_idx < get_nb_routes(); route_idx++)
        {
            if (finish_at_vertex(route_idx, vertex1))
            {
                nb_routes_ending_at_v1++;
            }
            if (start_at_vertex(route_idx, vertex2))
            {
                nb_routes_starting_at_v2++;
            }
        }

        return nb_routes_ending_at_v1 > 0 && nb_routes_starting_at_v2 > 0 &&
               get_vertex_point(vertex1) == get_vertex_point(vertex2);
    }

    bool is_cyclic_route(int route_idx)
    {
        std::vector<int> &route_vertices = get_route_vertices(route_idx);
        return _vertex_to_point[route_vertices[0]] == _vertex_to_point[route_vertices[route_vertices.size() - 1]];
    }

    bool arc_belongs_to_route(int route_idx, int arc_idx)
    {
        for(auto arc : _route_arcs[route_idx])
        {
            if (arc.idx == arc_idx)
                return true;
        }
        return false;
    }
};

#endif