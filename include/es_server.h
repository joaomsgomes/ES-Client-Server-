#ifndef ES_SERVER_H
#define ES_SERVER_H

#include <netinet/in.h>
#include <stdbool.h>
#include <pthread.h>


#define MYPORT "58084"

#define MAX_USERS 1000
#define MAX_EVENTS 999
#define MAX_RESERVATIONS 10000
#define PASSWORD_LEN 8
#define UID_LEN 6
#define EVENT_NAME_LEN 10
#define FILENAME_LEN 24
#define MAX_FILE_SIZE 10000000 // 10 MB
#define ARRAY_SIZE 128
#define DATE_STR_LEN 10 // dd-mm-yyyy
#define TIME_STR_LEN 5  // HH:MM

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
    char uid[UID_LEN + 1];
    int eid;
    int num_seats;
} Reservation;


#endif