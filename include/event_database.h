#ifndef EVENT_DATABASE_H
#define EVENT_DATABASE_H

#include "es_server.h"

// Event operations
int create_event(Event *ev);
int get_event(int eid, Event *ev);
int close_event(const char* uid, int eid);
int get_event_state(int eid);
int get_event_seats(int eid, int *total_seats, int *reserved_seats);
int is_event_owner(const char *uid, int eid);

// Reservation operations
int create_reservation(int eid, const char *uid, int num_seats);

#endif