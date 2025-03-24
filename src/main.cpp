#include "Data.hpp"
#include "Model.hpp"
#include "Combinations.hpp"

int main(int argc, char *argv[])
{
    Data data(argv[1]);
    data.print_data();

    Combinations comb;
    comb.init(data);

    // Model model;
    // model.init(data);
    return 0;
}
