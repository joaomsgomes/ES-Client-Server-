#ifndef ES_SERVER_H
#define ES_SERVER_H

#include <netinet/in.h>
#include <stdbool.h>




#define MYPORT "58000"

#define MAX_USERS 1000
#define MAX_EVENTS 999
#define MAX_RESERVATIONS 10000
#define PASSWORD_LEN 8
#define UID_LEN 6
#define EVENT_NAME_LEN 10
#define FILENAME_LEN 24
#define MAX_FILE_SIZE 10000000 // 10 MB
#define ARRAY_SIZE 128
#define DATE_STR_LEN 10 

// Estruturas de dados

typedef struct {
    char uid[UID_LEN + 1];
    char password[PASSWORD_LEN + 1];
    bool logged_in;
} User;

typedef struct {
    int eid;
    char uid[UID_LEN + 1];
    char name[EVENT_NAME_LEN + 1];
    char date[DATE_STR_LEN + 1]; // dd-mm-yyyy
    int total_seats;
    int reserved_seats;
    char filename[FILENAME_LEN + 1];
    long file_size;
    unsigned char *filedata;
    int state;
} Event;

typedef struct {
    char uid[UID_LEN + 1];
    int eid;
    int num_seats;
} Reservation;

// Funções principais do servidor
void init_server(int port, bool verbose);
void* udp_server_thread(void* arg);
void* tcp_server_thread(void* arg);


// Gestão de eventos
void init_event_system();
int create_event(Event *ev);
int close_event(const char* uid, int eid);
int get_event(int eid, Event *ev);
int get_event_state(int eid);
int is_date_in_past(const char *event_date);
int is_event_owner(const char *uid, int eid);
Event** list_all_events(int* count);
Event** list_user_events(const char* uid, int* count);

// Gestão de reservas
bool make_reservation(const char* uid, int eid, int num_people, int* available);
Reservation** list_user_reservations(const char* uid, int* count);

#endif