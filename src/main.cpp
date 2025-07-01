#include "Data.hpp"
#include "Model.hpp"
#include "Combinations.hpp"
#include "Heuristic.hpp"

int main(int argc, char *argv[])
{
    Data data(argv[1]);
    data.print_data();

    Combinations comb (data, 1, "static", 1); 

    // Heuristic heuristic;
    // heuristic.create_initial_combinations(data);

    // if (read_input(argc, argv))
    // {
    //     if (method == "model")
    //     {
        //    Model model;
        //    model.initialize(data);
        //    model.run(data);
    //     }
    //     else if (method == "enum")
    //     {
    //         Combinations comb (data, threads, scheduling, strategy); 
    //     }
    //     else if (method == "heuristic")
    //     {
    //         ...
    //     }
    // }

    return 0;
}
