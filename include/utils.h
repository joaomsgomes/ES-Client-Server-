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
bool validate_filename(const char* filename);
bool validate_seats(int seats);
bool validate_eid(int eid);
bool validate_num_people(int people);

// ============ CONSTANTS ============

#define MAX_BUFFER_SIZE 65536
#define MAX_PATH_LENGTH 256
#define MAX_COMMAND_LENGTH 512


// ============ DATE FUNCTIONS ============

void get_current_date(char* buffer);
int compare_dates(const char* date1, const char* date2);


// ============ SAFE I/O FUNCTIONS ============

ssize_t safe_read(int fd, void* buffer, size_t n);
ssize_t safe_write(int fd, const void* buffer, size_t n);


// ============ STRING UTILITIES ============

void trim_newline(char* str);
void safe_strcpy(char* dest, const char* src, size_t size);


// ============ FILE UTILITIES ============

bool file_exists(const char* filename);
long get_file_size(const char* filename);
bool ensure_directory(const char* path);


// ============ ERROR HANDLING ============

void print_error(const char* function_name, const char* message);
void debug_print(const char* format, ...);


// ============ USER INTERFACE ============

void show_help();


#endif // UTILS_H