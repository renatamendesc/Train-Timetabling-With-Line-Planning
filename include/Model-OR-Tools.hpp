#ifndef MODEL_OR_TOOLS_HPP
#define MODEL_OR_TOOLS_HPP

#include "Data.hpp"
#include <vector>

class ModelORTools
{
    public:
        void initialize(Data &data, int threads);
        void add_variables(Data &data);
        void add_constraints(Data &data);
};

#endif