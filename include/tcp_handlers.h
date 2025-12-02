#ifndef TCP_HANDLERS_H
#define TCP_HANDLERS_H

#include <sys/types.h>

/**
 * Handler para comando CREATE (CRE)
 * Protocolo: CRE UID password name date attendance Fname Fsize Fdata\n
 * Resposta: RCE status [EID]\n
 */
void handle_create_event(int client_fd, char* buffer, ssize_t n);

/**
 * Handler para comando CLOSE (CLS)
 * Protocolo: CLS UID password EID\n
 * Resposta: RCL status\n
 */
void handle_close_event(int client_fd, char* buffer, ssize_t n);

/**
 * Handler para comando LIST (LST)
 * Protocolo: LST\n
 * Resposta: RLS N [EID name date total_seats reserved_seats]\n
 */
void handle_list_events(int client_fd, char* buffer, ssize_t n);

/**
 * Handler para comando SHOW (SED)
 * Protocolo: SED EID\n
 * Resposta: RSE status [filename filesize filedata]\n
 */
void handle_show_event(int client_fd, char* buffer, ssize_t n);

/**
 * Handler para comando RESERVE (RID)
 * Protocolo: RID UID password EID seats\n
 * Resposta: RRI status\n
 */
void handle_reserve_seats(int client_fd, char* buffer, ssize_t n);

/**
 * Handler para comando CHANGE PASSWORD (CPS)
 * Protocolo: CPS UID old_password new_password\n
 * Resposta: RCP status\n
 */
void handle_change_password(int client_fd, char* buffer, ssize_t n);

/**
 * Processa comando TCP recebido e chama o handler apropriado
 */
void process_tcp_command(int client_fd, char* buffer, ssize_t n);

#endif
