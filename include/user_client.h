#ifndef USER_CLIENT_H
#define USER_CLIENT_H

#include <stdbool.h>

#define BUFFER_SIZE 65536
#define CMD_BUFFER 256

// Estado do cliente
typedef struct {
    char uid[7];
    char password[9];
    bool logged_in;
    char server_ip[16];
    int server_port;
    int udp_socket;
} ClientState;

// Comandos disponíveis
void cmd_login(ClientState* state, const char* uid, const char* password);
void cmd_logout(ClientState* state);
void cmd_unregister(ClientState* state);
void cmd_change_password(ClientState* state, const char* old_pass, const char* new_pass);
void cmd_create_event(ClientState* state, const char* name, const char* filename, 
                      const char* date, int seats);
void cmd_close_event(ClientState* state, int eid);
void cmd_list_events(ClientState* state);
void cmd_my_events(ClientState* state);
void cmd_show_event(ClientState* state, int eid);
void cmd_reserve(ClientState* state, int eid, int num_people);
void cmd_my_reservations(ClientState* state);
void cmd_exit(ClientState* state);

// Funções auxiliares
void init_client(ClientState* state, const char* ip, int port);
void parse_command(ClientState* state, const char* command);
void display_help();

#endif