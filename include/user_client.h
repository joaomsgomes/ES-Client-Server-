#ifndef USER_CLIENT_H
#define USER_CLIENT_H

#include <stdbool.h>
#include <stddef.h>

// ===== INICIALIZAÇÃO E CONEXÃO =====
bool init_udp_connection(const char* server_ip, const char* server_port);
int tcp_connect_to_server(void);
bool send_udp_receive_response(const char* message, char* response, size_t response_size);

// ===== COMANDOS UDP =====
void cmd_login(const char* uid, const char* password);
void cmd_logout(void);
void cmd_unregister(void);
void cmd_my_events(void);
void cmd_my_reservations(void);

// ===== COMANDOS TCP (a implementar) =====
void cmd_change_password(const char* old_pass, const char* new_pass);
void cmd_create_event(const char* name, const char* filename, 
                      const char* date, const char* time, int seats);
void cmd_close_event(const char* eid_str);
void cmd_list_events(void);
void cmd_show_event(const char *eid_str);
void cmd_reserve(const char *eid_str, int num_people);

// ===== FUNÇÕES AUXILIARES =====
void show_help(void);

#endif