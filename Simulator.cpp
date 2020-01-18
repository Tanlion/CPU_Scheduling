#include <iostream>
#include <fstream> // for output file to write simulation results to
#include <cstdlib> // for rand
#include <cmath>   // for log
#include "Simulator.h"

Simulator::Simulator(int s, int l, double s_t, double q)
{
    schedule = s;              // establish the type of simulation to perform
    end_condition = 10000;     // run until 10000 processes have been executed
    processes = 0;             // initialize amount of processes executed
    clock = 0.0;               // start clock at time 0
    cpu_usage = 0.0;           // initialize to accumulate service times of 10000 processes
    turnaround_time = 0.0;     // initialize to accumulate turnaround time of 10000 processes
    processes_in_queue = 0.0;  // initialize to accumulate every time an event occurs
    lambda = l;                // average arrival rate
    inverse_mu = s_t;          // average service time is inverse of average service rate
    quantum = q;               // time interval to allot for RR schedule
    cpu_idle = true;           // system initially is not running a process

    double new_t = 0, new_s;

    interArrivalTimeServiceTime(new_t, new_s);            // calculate inter-arrival and service times for first arrival to system

    Process* new_p = new Process(new_s, 0, new_s, new_s); // create the first process (assume it arrives at time 0)
    Event first_arrival(0, arrival, new_p);               // create first event

    event_q.push(first_arrival);                          // initialize event queue with first event corresponding to first arrival
}

void Simulator::simulate()
{
    while (processes != end_condition)
    {
        Event e = event_q.top();                                 // get first element in event queue
        event_q.pop();                                           // remove first element from event queue
        clock = e.time;                                          // get time event occurs (arrival arrives, departure departs, time slice is alloted)

        switch(e.type)
        {
            case arrival:
                {
                    switch(schedule)
                    {
                        case 1: scheduleFCFS(e); break;
                        case 2: scheduleSRTF(e); break;
                        case 3: scheduleHRRN(e); break;
                        case 4: scheduleRR(e); break;
                        default: break;
                    }
                } break;
            case departure:
                {
                    switch(schedule)
                    {
                        case 1: departureFCFS(e); break;
                        case 2: departureSRTF(e); break;
                        case 3: departureHRRN(e); break;
                        case 4: departureRR(e); break;
                    }

                    if (processes == end_condition)
                    {
                        std::ofstream fout("sim.data");
                        
                        turnaround_time /= 10000.0;    // calculate avg. turnaround time as sum(turnaround for 10,000 processes) / 10,000 processes
                        processes_in_queue /= 10000.0; // calculate avg. processes waiting in ready queue as processes_in_queue / total process
                        cpu_usage /= clock;            // calculate CPU utilization as sum(service_time) / completion time of last process
                        throughput = 10000.0 / clock;  // calculate throughput as 10,000 / completion time of last process

                        fout << turnaround_time << ", " << processes_in_queue << ", "
                             << cpu_usage << ", " << throughput;
                    }
                } break;
            case time_slice:
                {
                    handleTimeSlice(e);
                }
            default: break;
        }
    }

    std::cout << "Tq: " << turnaround_time << std::endl
              << "W:  " << processes_in_queue << std:: endl
              << "Rho: " << cpu_usage << std::endl
              << "Throughput: " << throughput << std::endl;
}

void Simulator::scheduleFCFS(const Event& e)
{
    if (cpu_idle)                     // FCFS, idle cpu means no process is in ready queue
    {
        on_cpu = e.p;                 // assign pointer of process to pointer representing cpu
        cpu_idle = false;             // cpu is no longer idle

        scheduleEvent(departure, clock + (e.p)->service_time, on_cpu);

        cpu_usage += (e.p)->service_time; // FCFS adds to CPU usage amount of time from when a process begins executing and completion
    }
    else ready_q.push(e.p);          // cpu is not idle, put process in ready queue

    scheduleEvent(arrival, clock, e.p);   // schedule next event that is set to arrive at some time from current time (clock)
}

void Simulator::scheduleSRTF(const Event& e)
{
    if (cpu_idle)
    {
        on_cpu = e.p;
        cpu_idle = false; // cpu is executing a process

        scheduleEvent(departure, on_cpu->completion_time, on_cpu);
    }
    else // determine whether the current arrival has a shorter remaining time than currently executing process (can preempt process executing)
    {
        on_cpu->remaining_time = on_cpu->completion_time - clock;

        if ((e.p)->remaining_time >= on_cpu->remaining_time)
        {
            ready_q_x.push(e.p); // current arrival does not preempt currently executing process
        }
        else // current arrival has a shorter remaining time to execute and preempts current executing process
        {
            Process* temp = on_cpu; // remove currently executing process from cpu
            on_cpu = e.p;           // assign new process to cpu

            scheduleEvent(departure, on_cpu->completion_time, on_cpu); // create a tentative departure event for preempting process

            ready_q_x.push(temp);
        }
    }

    scheduleEvent(arrival, clock, e.p); // schedule a new arrival to maintain simulation of process arrivals
}

void Simulator::scheduleHRRN(const Event& e)
{
    if (cpu_idle) // cpu is not executing a process, and there are no processes waiting in ready queue
    {
        cpu_idle = false; // cpu is no longer idle
        on_cpu = e.p;     // assign process to cpu to execute
        scheduleEvent(departure, clock + (e.p)->service_time /*(e.p)->completion_time*/, e.p);
    }
    else // cpu is executing a process but HRRN is not preemptive so process arriving must be placed in ready queue
    {
        ready_q_y.push_back(e.p); // list stl data structure is a doubly-linked list that can push and pop to and from front and back, respectively
    }

    scheduleEvent(arrival, clock, e.p); // Schedule the next arrival to simulate continuous arrival of processes
}

void Simulator::scheduleRR(const Event& e)
{
    if (cpu_idle) // cpu is not currently executing a process
    {
        cpu_idle = false; // cpu executing a process
        on_cpu = e.p;     // assign process to execute on cpu

        double r = (e.p)->remaining_time;

        if ((e.p)->service_time < quantum)
        {
            cpu_usage += (e.p)->remaining_time;
            (e.p)->remaining_time = 0;
            scheduleEvent(time_slice, clock + r, e.p);
        }
        else // process requires service for longer than the time alloted in a single quantum or timeslice
        {
            cpu_usage += quantum;
            (e.p)->remaining_time -= quantum; // by the end of the time slice to schedule, process will have executed quantum amount of time
            scheduleEvent(time_slice, clock + quantum, e.p);
        }
    }
    else ready_q.push(e.p); // cpu is currently working on a process. allow time slice for currently running process to handle arrival

    scheduleEvent(arrival, clock, e.p); // schedule an arrival to simulate continuous arrivals of processes
}

void Simulator::scheduleEvent(EventType e_t, double time, Process* p)
{
    switch(e_t)
    {
    case arrival:
        {
            double arrival_time = time,
                   service_time;

            interArrivalTimeServiceTime(arrival_time, service_time);

            Process* new_p = new Process(service_time,                   /* service_time */
                                         arrival_time,                   /* arrival_time */
                                         service_time,                   /* remaining_time */
                                         arrival_time+service_time);     /* completion_time */
            Event new_e(arrival_time, arrival, new_p);

            event_q.push(new_e);

            break;
        }
    case departure:
        {
            Event new_e(time, departure, p);
            event_q.push(new_e);
            break;
        }
    case time_slice:
        {
            Event new_e(time, time_slice, p);
            event_q.push(new_e);
            break;
        }
    }
}

void Simulator::departureFCFS(Event& d)
{
    turnaround_time += on_cpu->completion_time - on_cpu->arrival_time; // accumulate completion times (complete - arrival)
    ++processes; // count the number of processes that have been executed

    if (ready_q.empty()) {cpu_idle = true; cpu_usage += on_cpu->service_time;}
    else
    {
        processes_in_queue += ready_q.size(); // count the number of processes waiting in ready queue at this process's departure
        on_cpu = ready_q.front();
        cpu_usage += on_cpu->service_time; // if cpu was not idle, replace process with next process in ready queue, continue service
        scheduleEvent(departure, clock + on_cpu->service_time, on_cpu);
        ready_q.pop(); // remove first process from ready queue representing process has completed execution
    }

    delete d.p;
    d.p = NULL;
}

void Simulator::departureSRTF(Event& d)
{
    if (on_cpu == d.p) // only simulate a departure if the departure event represents a legitimate departure (process has completed)
    {
        if (on_cpu->completion_time == clock)
        {
            turnaround_time += on_cpu->completion_time - on_cpu->arrival_time; // accumulate completion times (complete - arrival)
            ++processes; // count the number of processes that have been executed
            cpu_usage += on_cpu->service_time;

            if (ready_q_x.empty()) cpu_idle = true; // cpu has no processes to execute if no processes are waiting
            else
            {
                processes_in_queue += ready_q_x.size(); // count the number of processes waiting in ready queue at this process's departure
                Process* top = ready_q_x.top();
                ready_q_x.pop();                        // remove the corresponding process from the min heap
                on_cpu = top;                           // retrieve next process from min heap (process with least amount of remaining time to execute)

                on_cpu->completion_time = clock + on_cpu->remaining_time;
                scheduleEvent(departure, clock + on_cpu->remaining_time, on_cpu);
            }
        }
    }
}

void Simulator::departureHRRN(Event& d)
{
    turnaround_time += (d.p)->completion_time - (d.p)->arrival_time;
    ++processes; // count the number of processes that have been executed
    cpu_usage += (d.p)->service_time;

    if (ready_q_y.empty()) cpu_idle = true; // if no processes are waiting to be executed, the cpu has no work to do
    else
    {
        processes_in_queue += ready_q_y.size(); // count the amount of processes waiting in ready queue at time of the current process's completion

        std::list<Process*>::iterator traverse, r; // use two iterators to traverse the elements of the list and to locate the process to remove
        double highest_response = 0.0; // initialize to 0.0 to find the highest response ratio

        for (traverse = ready_q_y.begin(); traverse != ready_q_y.end(); ++traverse)
        {
            double response_ratio = ((clock - (*traverse)->arrival_time) + (*traverse)->service_time) / (*traverse)->service_time; // (Tw + Ts) / Ts

            if (response_ratio > highest_response)
            {
                highest_response = response_ratio; // use to find process with highest response ratio
                r = traverse; // maintain a pointer to the process with highest response ratio
            }
        }

        Process* next = *r;  // preserve process to execute next
        ready_q_y.erase(r); // remove from the ready queue the pointer to the process found with highest response ratio
        on_cpu = next;      // assign process to cpu to execute

        on_cpu->completion_time = clock + on_cpu->service_time;    // establish completion time of this process
        scheduleEvent(departure, on_cpu->completion_time, on_cpu); // create a departure event for the process assigned to the cpu
    }

    delete d.p;    // deallocate memory allocated to the process
    d.p = NULL; // prevent memory leaks
}

void Simulator::departureRR(Event& d)
{
    turnaround_time += (d.p)->completion_time - (d.p)->arrival_time;
    ++processes; // count the amount of processes executed

    if (ready_q.empty()) cpu_idle = true; // no other processes are waiting
    else
    {
        processes_in_queue += ready_q.size(); // count the amount of processes waiting in ready queue

        on_cpu = ready_q.front(); // get first process waiting in queue and assign it to the cpu to execute
        ready_q.pop();            // remove first element from ready queue

        double r = on_cpu->remaining_time;

        if (on_cpu->remaining_time < quantum)
        {
            cpu_usage += on_cpu->remaining_time;
            on_cpu->completion_time = clock + on_cpu->remaining_time;
            on_cpu->remaining_time = 0; // process will complete by the end of the next time slice allotted to it
            scheduleEvent(time_slice, clock + r, on_cpu);
        }
        else
        {
            cpu_usage += quantum;
            on_cpu->completion_time = clock + quantum;
            on_cpu->remaining_time -= quantum;
            scheduleEvent(time_slice, clock + quantum, on_cpu);
        }
    }
}

// Only create a time slice, if the remaining time on a process is greater than the duration of a time slice
void Simulator::handleTimeSlice(Event& t)
{
    if ((t.p)->remaining_time == 0) scheduleEvent(departure, clock, t.p); // departure function will handle assigning next process to cpu
    else // current process requires more execution prior to completing and it must be determined whether it is preempted by another process or not
    {
        if (ready_q.empty())
        {
            //(t.p)->completion_time = clock + (t.p)->remaining_time;

            double r = (t.p)->remaining_time;

            if ((t.p)->remaining_time < quantum)
            {
                cpu_usage += r;
                (t.p)->completion_time = clock + (t.p)->remaining_time;
                (t.p)->remaining_time = 0;
                scheduleEvent(time_slice, clock + r, t.p);
            }
            else
            {
                cpu_usage += quantum;
                (t.p)->completion_time = clock + quantum;
                (t.p)->remaining_time -= quantum;
                scheduleEvent(time_slice, clock + quantum, t.p);
            }
        }
        else // at least one process waiting in ready queue and as the current process's quantum has expired, it is preempted by the waiting process
        {
            ready_q.push(t.p); // current executing process is preempted by first process waiting in ready queue

            Process* next = ready_q.front(); // retrieve the next process
            ready_q.pop();                   // remove process waiting in ready queue from ready queue
            on_cpu = next;                   // assign process that was waiting in ready queue to cpu

            double r = on_cpu->remaining_time;

            if (on_cpu->remaining_time < quantum)
            {
                cpu_usage += r;
                on_cpu->completion_time = clock + on_cpu->remaining_time;
                on_cpu->remaining_time = 0;
                scheduleEvent(time_slice, clock + r, on_cpu);
            }
            else
            {
                cpu_usage += quantum;
                on_cpu->completion_time = clock + quantum;
                on_cpu->remaining_time -= quantum;
                scheduleEvent(time_slice, clock + quantum, on_cpu);
            }
        }
    }
}

void Simulator::interArrivalTimeServiceTime(double& arrival_time, double& service_time)
{
    double probability = static_cast<double>(rand())/RAND_MAX;

    // prevent undefined behavior or service time of 0
    while (probability == 1 || probability == 0) probability = static_cast<double>(rand())/RAND_MAX;

    arrival_time += (-1.0/lambda) * log(1-probability);
    service_time = (-1.0*inverse_mu) * log(1-probability);
}
