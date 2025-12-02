#ifndef USER_CLIENT_H
#define USER_CLIENT_H

#include <stdbool.h>

/**
 * User Client Module
 * 
 * Cliente para comunicação com o Event Server (ES)
 * Cada instância do programa `./user` representa UM utilizador
 * O estado do cliente é mantido internamente como variável global estática
 * 
 * Para múltiplos utilizadores, execute múltiplas instâncias do programa:
 *   Terminal 1: ./user -n 127.0.0.1 -p 58000
 *   Terminal 2: ./user -n 127.0.0.1 -p 58000
 */

// ===== INICIALIZAÇÃO =====
bool init_udp_connection(const char* server_ip, const char* server_port);

// ===== COMANDOS UDP =====
void cmd_login(const char* uid, const char* password);
void cmd_logout(void);
void cmd_unregister(const char* uid, const char* password);
void cmd_my_events(void);
void cmd_my_reservations(void);

// ===== COMANDOS TCP (a implementar) =====
void cmd_change_password(const char* old_pass, const char* new_pass);
void cmd_create_event(const char* name, const char* filename, 
                      const char* date, int seats);
void cmd_close_event(int eid);
void cmd_list_events(void);
void cmd_show_event(int eid);
void cmd_reserve(int eid, int num_people);

// ===== FUNÇÕES AUXILIARES =====
void show_help(void);

#endif // USER_CLIENT_H