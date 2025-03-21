#include "Data.hpp"
#include "Model.hpp"

int main(int argc, char *argv[])
{
    Data data(argv[1]);
    data.print_data();

    Model model;
    model.init(data);
    return 0;
}
