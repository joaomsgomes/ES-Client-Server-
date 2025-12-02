#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <time.h>

// Inicialização do sistema de ficheiros
void init_file_system();

// Gestão de directorias
int create_user_directory(const char *uid);
int create_event_directory(int eid);

// Gestão de ficheiros de utilizador
int create_login_file(const char *uid);
int erase_login_file(const char *uid);
int is_logged_in(const char *uid);
int write_password_file(const char *uid, const char *password);
int read_password_file(const char *uid, char *password);
int erase_password_file(const char *uid);

// Gestão de eventos
int get_next_eid();
int event_exists(int eid);
int user_exists(const char *uid);

// Utilitários de tempo
time_t date_to_timestamp(const char *date_str);
void get_current_timestamp(char *buffer, size_t size);

#endif
