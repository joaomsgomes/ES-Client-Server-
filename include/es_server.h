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

// Estruturas de dados

typedef struct {
    char uid[UID_LEN + 1];
    char password[PASSWORD_LEN + 1];
    bool logged_in;
} User;

typedef enum {
    EVENT_ACTIVE = 1,
    EVENT_PAST = 0,
    EVENT_SOLD_OUT = 2,
    EVENT_CLOSED = 3
} EventState;

typedef struct {
    int eid;
    char owner_uid[UID_LEN + 1];
    char name[EVENT_NAME_LEN + 1];
    char date[11]; // dd-mm-yyyy
    int total_seats;
    int reserved_seats;
    EventState state;
    char filename[FILENAME_LEN + 1];
    long file_size;
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

// Gestão de utilizadores
void init_user_system();
void cleanup_user_system();
bool validate_uid(const char* uid);
bool validate_password(const char* password);
bool register_user(const char* uid, const char* password);
int authenticate_user(const char* uid, const char* password);  // Returns: 1=OK, 0=not exists, -1=wrong password

// Gestão de eventos
int create_event(const char* uid, const char* name, const char* date, 
                 int seats, const char* filename, const char* filedata, long filesize);
bool close_event(const char* uid, int eid);
Event* get_event(int eid);
Event** list_all_events(int* count);
Event** list_user_events(const char* uid, int* count);

// Gestão de reservas
bool make_reservation(const char* uid, int eid, int num_people, int* available);
Reservation** list_user_reservations(const char* uid, int* count);

#endif