#include "Data.hpp"
#include "Model.hpp"
#include "Combinations.hpp"

int main(int argc, char *argv[])
{
    Data data(argv[1]);
    data.print_data();

    Combinations comb (data, std::stoi(argv[2]));

    // Model model;
    // model.initialize(data);
    // model.run(data);

    return 0;
}
