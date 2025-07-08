#include "MyIncumbentCallback.hpp"
#include <iostream>
#include <cmath>

using namespace std;

MyIncumbentCallback::MyIncumbentCallback(IloEnv env, std::chrono::steady_clock::time_point start)
    : IloCplex::IncumbentCallbackI(env), start_time(start) {}

void MyIncumbentCallback::main()
{
    best_solution_time = std::chrono::duration<double>(chrono::steady_clock::now() - start_time).count();
    std::cout << ">>> New best was found within " << best_solution_time << " seconds..." << std::endl;
}

IloCplex::CallbackI* MyIncumbentCallback::duplicateCallback() const
{
    return new (getEnv()) MyIncumbentCallback(*this);
}