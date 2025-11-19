#include "Data.hpp"
#include "Model-OR-Tools.hpp"
// #include "Model.hpp"
#include "Enumeration.hpp"
#include "Heuristic.hpp"

// ----------------------------------------------------------------- //
// Input: ./cbtu <instance> <method> <number of threads>
// ----------------------------------------------------------------- //

int main(int argc, char *argv[])
{
    // read instance
    if (argc < 2)
    {
        std::cout << "Error: Instance not provided!" << std::endl;
        return 1;
    }
    Data data(argv[1]);
    // data.print_data();

    // read method
    if (argc < 3)
    {
        std::cout << "Error: Method not provided!" << std::endl;
        return 1;
    }
    std::string method = argv[2];

    // // read number of threads
    // int threads = 1;
    // if (argc < 4)
    // {
    //     std::cout << "Number of threads not provided! - Using default number of threads: 1" << std::endl;
    // }
    // else
    // {
    //     threads = std::stoi(argv[3]);
    //     // verify whether number of threads is valid (surpasses capacity of the machine or is less than 1)
    //     if (threads > omp_get_max_threads() || threads < 1)
    //     {
    //         std::cout << "Error: Number of threads is not valid!" << std::endl;
    //         return 1;
    //     }
    // }

    // // display instance, method and number of threads
    // std::cout << std::endl << "\t================================================================" << std::endl;
    // std::cout << "\tSolving instance " << data.get_instance_name() << " with " << method << " and " << threads << " thread(s)..." << std::endl;
    // std::cout << "\t================================================================" << std::endl;
    // std::cout << "\t>> Instance: " << data.get_instance_name() << std::endl;
    // std::cout << "\t>> Method: " << method << std::endl;
    // std::cout << "\t>> Number of threads: " << threads << std::endl << std::endl;

    // execute selected method
    if (method == "model")
    {
        ModelORTools model;
        model.initialize(data, 1);
        model.create_full_model(data);
        model.execute_solver_for_full_model(data);
        model.get_value_of_variables(data);
        std::cout << std::endl << "-> Total time = " << model.current_sol.computational_time.count() << std::endl;
        model.current_sol.display_solution(data);
    }
    else if (method == "enum")
    {
        Enumeration enumeration(data, 1, 43200, 7200);
    }
    else if (method == "heuristic")
    {
        Heuristic heuristic(data, 1, 43200, 2400);
    }
    else
    {
        std::cout << "Did not provide a valid method!" << std::endl;
        return 1;
    }

    return 0;
}
