#include <Enumeration.hpp>

using namespace std;

Enumeration::Enumeration(Data &data, int nb_threads, int time_limit_complete, int time_limit_per_combination)
{
    this->time_limit_complete = time_limit_complete;
    this->time_limit_per_combination = time_limit_per_combination;

    this->nb_threads = nb_threads;
    this->nb_trains = data.get_nb_trains();

    start = chrono::steady_clock::now();
    execute_enumeration(data);
    end = chrono::steady_clock::now();

    cout << "All combination(s) tested!" << endl;
    // get number of feasible solutions
    cout << nb_feasible_combinations << "/" << total_nb_combinations << " were feasible combination(s)! - (" << setprecision(3) << (double(nb_feasible_combinations) / double(total_nb_combinations) ) * 100 << "\%)" << endl;
    if (nb_feasible_combinations == 0)
    {
        cout << "\tNo feasible solution was found... Instance is infeasible!" << endl;
        return;
    }

    if (!overall_best_sol.proven_optimal)
        cout << "\tCannot prove optimality!" << endl;

    // display best solution
    cout << endl << "-> Total time = " << chrono::duration<double>(end-start).count() << endl;
    overall_best_sol.display_solution(data);
}

void Enumeration::execute_enumeration(Data &data)
{
    // calculate total number of possible combinations
    if (comb.calculate_trips_combinations(data)) 
    {
        total_nb_combinations = 1;
        for (int i = 0; i < comb.all_trips_combinations.size(); i++)
        {
            total_nb_combinations *= comb.all_trips_combinations[i].size(); 
        }
        cout << endl << "Starting to test combinations... - Total number of combinations = " << total_nb_combinations << endl;
        execute_all_combinations(data);
    }
    else
    {
        cout << endl << ">> Too many combinations. Instance can't be solved by enumeration method!" << endl;
        exit(1);
    }
}

void Enumeration::execute_all_combinations(Data &data)
{
    // variable to assist in displaying progress
    int aux_progress = ceil(0.1 * total_nb_combinations);
    if (aux_progress == 0)
        aux_progress = 1;

    // vectors with model objects for each thread
    vector <ModelORTools> models(nb_threads);

    int counter_solved = 0;

    #pragma omp parallel num_threads(nb_threads)
    {
        int thread_id = omp_get_thread_num();
        #pragma omp for schedule(dynamic)
        for (unsigned long long count = 0; count < total_nb_combinations; count++)
        {
            // get current combination
            int idx = count;
            vector<int> indices(nb_trains);
            for (int k = nb_trains - 1; k >= 0; k--)
            {
                indices[k] = idx % comb.all_trips_combinations[k].size();
                idx /= comb.all_trips_combinations[k].size();
            }
            RoutesCombination current_combination;
            for (int k = 0; k < nb_trains; k++)
            {
                current_combination.push_back(comb.all_trips_combinations[k][indices[k]]);
            }

            if (comb.is_valid_combination(data, current_combination))
            {
                ModelORTools &model_thread = models[thread_id];
                model_thread.initialize(data, 1);
                model_thread.create_model_with_routes_constraints(data, current_combination);
                int feasible = model_thread.execute_solver_for_combination(data, overall_best_sol.obj_value, time_limit_per_combination, "enum");
                if (feasible)
                {
                    #pragma omp atomic
                    nb_feasible_combinations++;
                    #pragma omp critical
                    {
                        if (model_thread.best_sol.obj_value < overall_best_sol.obj_value)
                        {
                            overall_best_sol = model_thread.best_sol;
                        }
                    }
                }
            }
            #pragma omp atomic
            counter_solved++;

            // display progress
            if (counter_solved % aux_progress == 0)
                cout << counter_solved/aux_progress * 10 << "%" << " done - " << counter_solved << "/" << total_nb_combinations << " combination(s) tested! (Thread " << thread_id << ")" << endl;

            // verify time limit
            chrono::time_point<std::chrono::steady_clock> now = chrono::steady_clock::now();
            chrono::duration<double> current_time = now-start;
            if (current_time.count() >= time_limit_complete)
            {
                #pragma omp critical
                {
                    reached_time_limit(data, current_time);
                }
            }
        }
    }
}

void Enumeration::reached_time_limit(Data &data, chrono::duration<double> time)
{
    cout << endl << ">> Reached time limit!" << endl;
    cout << "\tTerminating process!" << endl;
    if (nb_feasible_combinations == 0)
    {
        cout << "\tNo feasible solution was found..." << endl;
        cout << endl << "-> Total time = " << time.count() << endl;
    }
    else
    {
        cout << "\tCannot prove optimality!" << endl;
        cout << endl << "-> Total time = " << time.count() << endl;
        overall_best_sol.display_solution(data);
    }
    exit(0);
}