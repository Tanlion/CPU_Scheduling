#ifndef EVENT_H
#define EVENT_H

#include "EventType.h"
#include "Process.h"

struct Event
{
    double time; // use for time of arrival, departure, etc.
    EventType type; // use to specify the type of event (arrival, departure, time slice)
    Process* p; // pointer to the corresponding process

    Event(double t, EventType e, Process* new_p) : time(t), type(e), p(new_p) {}
};

#endif // EVENT_H