#include "Data.hpp"
#include "Model.hpp"
#include "Combinations.hpp"
#include "Heuristic.hpp"

// ----------------------------------------------------------------- //
// Input: ./cbtu instances/<name-instance> <method> <max_nb_threads>
// ----------------------------------------------------------------- //

int main(int argc, char *argv[])
{
    Data data(argv[1]);
    data.print_data();

    // read method and max number of threads
    std::string method = argv[2];
    int threads = 4;
    if (argc > 3)
        threads = std::stoi(argv[3]);

    // execute selected method
    if (method == "model")
    {
        Model model;
        model.initialize(data);
        model.run(data);
    }
    else if (method == "enum")
    {
        Combinations comb (data, threads, 0);
    }
    else if (method == "heuristic")
    {
        Combinations comb (data, threads, 1);
    }
    else
    {
        std::cout << "Not a valid method!" << std::endl;
        return 1;
    }

    return 0;
}
