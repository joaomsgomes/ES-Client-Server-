#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

// ============ VALIDATION FUNCTIONS ============

bool validate_uid(const char* uid);
bool validate_password(const char* password);
bool validate_event_name(const char* name);
bool validate_date(const char* date);

// ============ CONSTANTS ============

#define MAX_BUFFER_SIZE 65536
#define MAX_PATH_LENGTH 256
#define MAX_COMMAND_LENGTH 512


// ============ DATE FUNCTIONS ============
void get_current_date(char* buffer);
int compare_dates(const char* date1, const char* date2);


#endif // UTILS_H