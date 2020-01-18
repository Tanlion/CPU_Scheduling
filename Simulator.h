#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <queue>       // for stl queue data structures
#include <vector>      // for container adapter vector in priority_queue
#include <list>
#include "Event.h"
#include "Process.h"
#include "EventType.h"

class Simulator
{
public:
    // Constructor
    Simulator(int, int, double, double);
    // Accessors
    void simulate(); // engine that runs simulation of CPU scheduling

private:
    // general schedule of an event (arrival, departure, time_slice)
    void scheduleEvent(EventType, double, Process* p);

    // CPU Schedule algorithms
    void scheduleFCFS(const Event& e);
    void scheduleSRTF(const Event& e);
    void scheduleHRRN(const Event& e);
    void scheduleRR(const Event& e);

    // responses to a process' completion depending on the type of cpu schedule in use
    void departureFCFS(Event& d);
    void departureSRTF(Event& d);
    void departureHRRN(Event& d);
    void departureRR(Event& d);

    // handles time slices for round robin scheduler
    void handleTimeSlice(Event& t);

    void interArrivalTimeServiceTime(double&, double&); // calculate the inter-arrival time of a process and its service time

    // Functor to equip priority_queue with means for interpreting priority of Event structs
    class EventCompare
    {
    public:
        bool operator()(Event& a, Event& b)
        {
            return a.time > b.time; // arrange events according to earliest event at front
        }
    };

    class SRTFCompare
    {
    public:
        bool operator()(Process* a, Process* b)
        {
            return a->remaining_time > b->remaining_time; // arrange processes according to process with least remaining execution time at front
        }
    };

    std::priority_queue<Event, std::vector<Event>, EventCompare> event_q;
    std::priority_queue<Process*, std::vector<Process*>, SRTFCompare> ready_q_x;
    std::queue<Process*> ready_q;
    std::list<Process*> ready_q_y; // use for HRRN to leverage constant removal of elements when retrieving next waiting process

    int schedule,              // type of schedule to simulate
        processes,             // count the amount of processes executed
        end_condition,         // total number of processes to process
        events;                // ask about whether this is a correct method for calculating avg. waiting processes in ready queue
    double clock,              // keep time of events
           cpu_usage,          // keep track of amount of time cpu is used
           turnaround_time,    // accumulate process's turnaround time
           throughput,         // processes / unit time
           processes_in_queue, // update every event
           lambda,             // average arrival rate
           inverse_mu,         // average service rate
           quantum,            // time interval length for RR schedule
           check_cpu_usage;
    bool cpu_idle;             // use to determine whether cpu is in use

    Process* on_cpu;           // pointer to process on cpu (maintain in preemptive schedulers)
};
#endif // SIMULATOR_H