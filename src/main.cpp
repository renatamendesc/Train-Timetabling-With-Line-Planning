#include "Data.hpp"
#include "Model.hpp"
#include "Combinations.hpp"
#include "Heuristic.hpp"

// Input must indicate instance and method to be used (model or enum). If enumeration was chosen,
// the user also must indicate de number of threads to be used, and the scheduling of thread scheduling
// (stactic or dynamic).
// Input example: ./cbtu instance/t1p5h1A.txt --method enum --threads 4 --scheduling dynamic --strategy 0

std::string method;
int threads = 1;
std::string scheduling = "static";
int strategy = 0;

int read_input (int argc, char *argv[])
{
    std::cout << std::endl;

    int error = 0;
    if (argc < 4)  // verify if method was informed
    {
        error = 1; // missing method parameter
    }
    else
    {
        if (std::string(argv[2]) == "--method")
        {
            if (std::string(argv[3]) == "model")
            {
                method = "model";
            }
            else if (std::string(argv[3]) == "enum")
            {
                method = "enum";

                if (argc < 6)  // number of threads was not informed
                {
                    error = 1; // missing threads parameters
                }
                else           // number of threads was informed
                {
                    if (std::string(argv[4]) == "--threads")
                    {
                        try
                        {
                            // verify if number of threads is valid
                            threads = std::stoi(argv[5]); 
                        }
                        catch (...) 
                        {
                            error = 2;
                        }
                    }
                }

                if (argc < 8)  // scheduling was not informed
                {
                    error = 1; // missing scheduling parameters
                }
                else           // scheduling was informed
                {
                    if (std::string(argv[6]) == "--scheduling")
                    {
                        // verify if scheduling is valid
                        if (std::string(argv[7]) == "static" || std::string(argv[7]) == "dynamic") 
                        {
                            scheduling = argv[7];
                        }
                        else
                        {
                            error = 2;
                        }

                    }
                }

                if (argc < 10)  // strategy was not informed
                {
                    error = 1; // missing strategy parameters
                }
                else           // strategy was informed
                {
                    if (std::string(argv[8]) == "--strategy")
                    {
                        // verify if strategy is valid
                        try
                        {
                            if (strategy == 0 || strategy == 1)
                            {
                                strategy = std::stoi(argv[9]);
                            }
                            else
                            {
                                error = 2;
                            }
                        }
                        catch (...) 
                        {
                            error = 2;
                        }

                        

                    }
                }
            }
            else
            {
                error = 2;
            }
        }
        else
        {
            error = 2;
        }
    }

    if (error == 0) // no errors
        return 1;

    if (error == 1)
    {
        std::cerr << "Error: Missing parameters!" << std::endl;
        return 0;
    }

    if (error == 2)
    {
        std::cerr << "Error: One or more arguments are invalid.\n";
        return 0;
    }

}

int main(int argc, char *argv[])
{
    Data data(argv[1]);
    data.print_data();

    Combinations comb (data, 1, "static", 0); 

    // Heuristic heuristic;
    // heuristic.create_initial_combinations(data);

    // if (read_input(argc, argv))
    // {
    //     if (method == "model")
    //     {
    //        Model model;
    //        model.initialize(data);
    //        model.run(data);
    //     }
    //     else if (method == "enum")
    //     {
    //         Combinations comb (data, threads, scheduling, strategy); 
    //     }
    // }

    return 0;
}
