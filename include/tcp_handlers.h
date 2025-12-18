#ifndef TCP_HANDLERS_H
#define TCP_HANDLERS_H

#include <sys/types.h>


void handle_create_event(int client_fd, char* buffer, ssize_t n);

//Handler para comando CLOSE (CLS)
void handle_close_event(int client_fd, char* buffer, ssize_t n);

//Handler para comando LIST (LST)
void handle_list_events(int client_fd, char* buffer, ssize_t n);

//Handler para comando SHOW (SED)
void handle_show_event(int client_fd, char* buffer, ssize_t n);

//Handler para comando RESERVE (RID)
void handle_reserve_seats(int client_fd, char* buffer, ssize_t n);

//Handler para comando CHANGE_PASS (CPS)
void handle_change_password(int client_fd, char* buffer, ssize_t n);


#endif
