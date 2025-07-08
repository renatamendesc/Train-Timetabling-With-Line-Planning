
#ifndef MY_INCUMBENT_CALLBACK_HPP
#define MY_INCUMBENT_CALLBACK_HPP

#include <ilcplex/ilocplex.h>
#include <chrono>

class MyIncumbentCallback : public IloCplex::IncumbentCallbackI {
    public:
        MyIncumbentCallback(IloEnv env, std::chrono::steady_clock::time_point start);

        static double best_solution_time;

        void main() override;
        IloCplex::CallbackI* duplicateCallback() const override;

    private:
        std::chrono::steady_clock::time_point start_time;
};

#endif