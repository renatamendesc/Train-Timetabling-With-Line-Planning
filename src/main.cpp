#include "Data.hpp"
#include "Model.hpp"
#include "Combinations.hpp"
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

    // read number of threads
    int threads = 1;
    if (argc < 4)
    {
        std::cout << "Number of threads not provided! - Using default number of threads: 1" << std::endl;
    }
    threads = std::stoi(argv[3]);
    // verify whether number of threads is valid (surpasses capacity of the machine or is less than 1)
    if (threads > omp_get_max_threads() || threads < 1)
    {
        std::cout << "Error: Number of threads is not valid!" << std::endl;
        return 1;
    }

    // display instance, method and number of threads
    std::cout << std::endl << "\t================================================================" << std::endl;
    std::cout << "\tSolving instance " << data._instance_name << " with " << method << " and " << threads << " thread(s)..." << std::endl;
    std::cout << "\t================================================================" << std::endl;
    std::cout << "\t>> Instance: " << data._instance_name << std::endl;
    std::cout << "\t>> Method: " << method << std::endl;
    std::cout << "\t>> Number of threads: " << threads << std::endl << std::endl;

    // execute selected method
    if (method == "model")
    {
        Model model;
        model.initialize(data);
        model.run(data, threads);
    }
    else if (method == "enum" || method == "heuristic")
    {
        Combinations comb (data, threads, method);
    }
    else
    {
        std::cout << "Not a valid method was provided!" << std::endl;
        return 1;
    }

    return 0;
}
