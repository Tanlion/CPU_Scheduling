#include <iostream>
#include <cstdlib>
#include "Simulator.h"

int main(int argc, char* argv[])
{
    if (argc != 5)
    {
        std::cout << "usage: " << argv[0] << ", scheduler (1-4), "
                  << "lambda, Ts, Quantum interval\n";
        exit(-1);
    }

    int scheduler = atoi(argv[1]);
    if (scheduler < 1 || scheduler > 4)
    {
        std::cout << "Scheduler must be designated by a number 1-4\n"
                  << "1. First Come First Serve\n"
                  << "2. Shortest Remaining Time First\n"
                  << "3. Highest Response Ratio Next\n"
                  << "4. Round Robin\n"
                  << "Please enter one of the available selections.\n";
        exit(-1);
    }

    int lambda = atoi(argv[2]);
    if (lambda < 1)
    {
        std::cout << "Average arrival rate must be an integer "
                  << "greater than 1.\n";
        exit(-1);
    }

    double service_time = atof(argv[3]);
    if (service_time < 0)
    {
        std::cout << "Average service time cannot be negative.\n";
        exit(-1);
    }

    double quantum = atof(argv[4]);
    if (quantum < 0)
    {
        std::cout << "Quantum (time slice) cannot be negative.\n";
        exit(-1);
    }

    Simulator cpu_scheduler(scheduler, lambda, service_time, quantum);

    cpu_scheduler.simulate();

    return 0;
}
