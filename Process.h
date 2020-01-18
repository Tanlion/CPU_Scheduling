#ifndef PROCESS_H
#define PROCESS_H

struct Process
{
    double service_time,
           arrival_time,
           remaining_time,
           completion_time; // what time the process completes

    Process(double s_t, double a_t, double r_t, double c_t) :
            service_time(s_t), arrival_time(a_t), remaining_time(r_t),
            completion_time(c_t) {}
};

#endif // PROCESS_H