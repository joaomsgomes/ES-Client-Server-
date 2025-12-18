#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

// ============ VALIDATION FUNCTIONS ============

bool validate_uid(const char* uid);
bool validate_password(const char* password);
bool validate_event_name(const char* name);
bool validate_datetime_format(const char* date, const char* time);
bool validate_datetime_range(const char* date, const char* time);
bool validate_datetime(const char* date, const char* time);

// ============ CONSTANTS ============

#define MAX_BUFFER_SIZE 65536
#define MAX_PATH_LENGTH 256
#define MAX_COMMAND_LENGTH 512


// ============ DATE FUNCTIONS ============
void get_current_datetime(char* buffer);
int compare_dates(const char* date1, const char* date2);
bool is_date_before_now(const char* date);


#endif