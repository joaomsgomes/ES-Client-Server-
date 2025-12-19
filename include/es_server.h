#ifndef ES_SERVER_H
#define ES_SERVER_H

#include <netinet/in.h>
#include <stdbool.h>
#include <pthread.h>
#include "common_defs.h"


#define MYPORT "58084"

// Constantes específicas do servidor
#define MAX_USERS 1000
#define MAX_EVENTS 999
#define MAX_RESERVATIONS 10000
#define ARRAY_SIZE 128

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
    char time[TIME_STR_LEN + 1]; // HH:MM
    int total_seats;
    int reserved_seats;
    char filename[FILENAME_LEN + 1];
    long file_size;
    unsigned char *filedata;
    int state;
} Event;

typedef struct {
    int eid;
    int num_seats;
    char date[DATE_STR_LEN + 1];
    char time[TIME_WITH_SECONDS_BUFFER_LEN];  // HH:MM:SS
    char datetime[DATETIME_WITH_SECONDS_LEN];  
} Reservation;


#endif