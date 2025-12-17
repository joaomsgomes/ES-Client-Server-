#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "../include/protocol.h"
#include "../include/user_client.h"
#include "../include/utils.h"

#define INPUT_BUFFER_SIZE 256
#define PASSWORD_LEN 8
#define UID_LEN 6
#define DATE_STR_LEN 10
#define TIME_STR_LEN 5
#define EVENT_NAME_LEN 10

// Estado global do cliente
static struct {
    int udp_socket;
    struct addrinfo *server_addr;
    char logged_uid[7];
    char logged_password[9];
    bool is_logged_in;
    char server_ip[128];
    char server_tcp_port[8];
} client_state = {-1, NULL, "000000", "00000000", false, "localhost", "58001"};

/**
 * Inicializa conexão UDP com o servidor
 */
bool init_udp_connection(const char* server_ip, const char* server_port) {
    struct addrinfo hints;
    int errcode;
    
    // Guardar IP e porta TCP do servidor (mesma porta que UDP)
    strncpy(client_state.server_ip, server_ip, sizeof(client_state.server_ip) - 1);
    strncpy(client_state.server_tcp_port, server_port, sizeof(client_state.server_tcp_port) - 1);
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    
    errcode = getaddrinfo(server_ip, server_port, &hints, &client_state.server_addr);
    if (errcode != 0) {
        fprintf(stderr, "Error: getaddrinfo: %s\n", gai_strerror(errcode));
        return false;
    }
    
    client_state.udp_socket = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket (fd)
    if (client_state.udp_socket == -1) {
        perror("Error creating UDP socket");
        freeaddrinfo(client_state.server_addr);
        return false;
    }
    
    // Configurar timeout de 5 segundos para recvfrom
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    
    if (setsockopt(client_state.udp_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error setting socket timeout");
        close(client_state.udp_socket);
        freeaddrinfo(client_state.server_addr);
        return false;
    }
    
    printf("Connected to ES server at %s:%s\n", server_ip, server_port);
    return true;
}

/**
 * Envia mensagem UDP e recebe resposta
 */
bool send_udp_receive_response(const char* message, char* response, size_t response_size) {
    
    ssize_t n;
    struct sockaddr_in server_sockaddr;
    socklen_t addrlen;
    
    // Enviar mensagem
    n = sendto(client_state.udp_socket, message, strlen(message), 0,
               client_state.server_addr->ai_addr, client_state.server_addr->ai_addrlen);
    
    if (n == -1) {
        perror("Error sending UDP message");
        return false;
    }
    
    // Receber resposta
    addrlen = sizeof(server_sockaddr);
    n = recvfrom(client_state.udp_socket, response, response_size - 1, 0,
                 (struct sockaddr*)&server_sockaddr, &addrlen);
    
    if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            fprintf(stderr, "Error: Server response timeout (no response received)\n");
        } else {
            perror("Error receiving UDP response");
        }
        return false;
    }
    
    response[n] = '\0';
    return true;
}

/**
 * Estabelece conexão TCP com o servidor
 * Retorna o socket TCP conectado, ou -1 em caso de erro
 */
int tcp_connect_to_server() {
    struct addrinfo hints, *tcp_res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    int errcode = getaddrinfo(client_state.server_ip, client_state.server_tcp_port, &hints, &tcp_res);
    if (errcode != 0) {
        fprintf(stderr, "Error: getaddrinfo TCP: %s\n", gai_strerror(errcode));
        return -1;
    }
    
    int tcp_socket = socket(tcp_res->ai_family, tcp_res->ai_socktype, tcp_res->ai_protocol);
    if (tcp_socket == -1) {
        perror("Error creating TCP socket");
        freeaddrinfo(tcp_res);
        return -1;
    }
    
    
    if (connect(tcp_socket, tcp_res->ai_addr, tcp_res->ai_addrlen) == -1) {
        perror("Error connecting to TCP server");
        close(tcp_socket);
        freeaddrinfo(tcp_res);
        return -1;
    }
    
    freeaddrinfo(tcp_res);
    return tcp_socket;
}

/**
 * Comando: login UID password
 */
void cmd_login(const char* uid, const char* password) {
    char message[64];
    char response[64];
    
    // Verificar se já existe outro utilizador logado neste terminal
    if (client_state.is_logged_in) {
        if (strcmp(client_state.logged_uid, uid) != 0) {
            printf("Error: User %s is already logged in. Please logout first.\n", 
                   client_state.logged_uid);
            return;
        } else {
            // Mesmo utilizador já está logado
            printf("User %s is already logged in\n", uid);
            return;
        }
    }
    
    // Validar formato antes de enviar
    if (strlen(uid) != 6) {
        printf("Error: UID must be exactly 6 digits\n");
        return;
    }
    
    if (strlen(password) != 8) {
        printf("Error: Password must be exactly 8 alphanumeric characters\n");
        return;
    }
    
    // Construir mensagem: "LIN UID password\n"
    snprintf(message, sizeof(message), "%s %s %s\n", CMD_LOGIN, uid, password);
    
    // Enviar e receber resposta
    if (!send_udp_receive_response(message, response, sizeof(response))) {
        printf("Error: Communication with server failed\n");
        return;
    }
    
    // Parse resposta: "RLI status\n"
    char rsp_code[4], status[4];
    if (sscanf(response, "%3s %3s", rsp_code, status) != 2) {
        printf("Error: Invalid response format\n");
        return;
    }
    
    // Processar status
    if (strcmp(status, STATUS_OK) == 0) {
        strcpy(client_state.logged_uid, uid);
        strcpy(client_state.logged_password, password);
        strcpy(client_state.logged_password, password);
        client_state.is_logged_in = true;
        printf("User logged in successfully\n");
        
    } else if (strcmp(status, STATUS_REG) == 0) {
        strcpy(client_state.logged_uid, uid);
        strcpy(client_state.logged_password, password);
        client_state.is_logged_in = true;
        printf("New user registered\n");
        
    } else if (strcmp(status, STATUS_NOK) == 0) {
        printf("Incorrect login attempt\n");
        
    } else if (strcmp(status, STATUS_ERR) == 0) {
        printf("Error: Invalid format or syntax\n");
        
    } else {
        printf("Unknown response: %s\n", response);
    }
}

void cmd_logout() {
    char message[64];
    char response[64];

    snprintf(message, sizeof(message), "%s %s %s\n", 
             CMD_LOGOUT, client_state.logged_uid, client_state.logged_password);
    
    if (!send_udp_receive_response(message, response, sizeof(response))) {
        printf("Error: Communication with server failed\n");
        return;
    }

    char rsp_code[4], status[4];
    if (sscanf(response, "%3s %3s", rsp_code, status) != 2) {
        printf("Error: Invalid response format\n");
        return;
    }

    if (strcmp(status, STATUS_OK) == 0) {
        client_state.is_logged_in = false;
        printf("User logged out successfully\n");
        
    } else if (strcmp(status, STATUS_NOK) == 0) {
        printf("Error: User not logged in\n");
        
    } else if (strcmp(status, STATUS_UNR) == 0) {
        printf("Error: User not registered\n");
        
    } else if (strcmp(status, STATUS_WRP) == 0) {
        printf("Error: Wrong password\n");

    } else if (strcmp(status, STATUS_ERR) == 0) {
    printf("Error: Invalid request format\n");
        
    } else {
        printf("Unknown response: %s\n", response);
    }
}

void cmd_unregister() {
    // Implementar comando unregister similar a login/logout
    char message[64];
    char response[64];

    snprintf(message, sizeof(message), "%s %s %s\n", CMD_UNREGISTER, client_state.logged_uid, client_state.logged_password);

    if (!send_udp_receive_response(message, response, sizeof(response))) {
        printf("Error: Communication with server failed\n");
        return;
    }

    char rsp_code[4], status[4];
    if (sscanf(response, "%3s %3s", rsp_code, status) != 2) {
        printf("Error: Invalid response format\n");
        return;
    }

    if (strcmp(status, STATUS_OK) == 0) {
        client_state.is_logged_in = false;
        strcpy(client_state.logged_uid, "000000");
        strcpy(client_state.logged_password, "00000000");
        printf("User unregistered successfully\n");
        
    } else if (strcmp(status, STATUS_NOK) == 0) {
        printf("Error: Not logged in\n");
        
    } else if (strcmp(status, STATUS_UNR) == 0) {
        printf("Error: User not registered\n");
        
    } else if (strcmp(status, STATUS_WRP) == 0) {
        printf("Error: Wrong password\n");

    } else if (strcmp(status, STATUS_ERR) == 0) {
        printf("Error: Invalid request format\n");
        
    } else {
        printf("Unknown response: %s\n", response);
    }
}


void cmd_create_event(const char* name, const char* event_fname, const char* event_date, const char* event_time, int num_attendees) {
    // Verificar se está logado
    if (!client_state.is_logged_in) {
        printf("Error: You must be logged in to create events\n");
        return;
    }
    
    // Validar parâmetros
    if (!validate_event_name(name)) {
        printf("Error: Invalid event name (max 10 alphanumeric characters)\n");
        return;
    }
    
    if (!validate_datetime_format(event_date, event_time)) {
        printf("Error: Invalid format - use dd-mm-yyyy hh:mm\n");
        return;
    }
    
    if (!validate_datetime_range(event_date, event_time)) {
        printf("Error: Date/time out of valid range (day: 1-31, month: 1-12, year: 1900-2100, hour: 0-23, minute: 0-59)\n");
        return;
    }
    
    // Validar que a data/hora não é passada
    char full_datetime[20];
    snprintf(full_datetime, sizeof(full_datetime), "%s %s", event_date, event_time);
    if (is_date_before_now(full_datetime)) {
        printf("Error: Event date cannot be in the past\n");
        return;
    }

    
    if (num_attendees < 10 || num_attendees > 999) {
        printf("Error: Attendance must be between 10 and 999\n");
        return;
    }
    
    // Abrir e ler ficheiro (deve existir na mesma pasta)
    char dir[128];
    snprintf(dir, sizeof(dir), "events/%s", event_fname);
    FILE *file = fopen(dir, "rb");
    if (!file) {
        printf("Error: Cannot open file '%s' (must exist in current folder)\n", dir);
        perror("Details");
        return;
    }
    
    // Obter tamanho do ficheiro
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (filesize <= 0 || filesize > 10000000) { // Max 10MB
        printf("Error: File size must be between 1 byte and 10MB (current: %ld bytes)\n", filesize);
        fclose(file);
        return;
    }
    
    // Alocar memória para dados do ficheiro
    unsigned char *filedata = malloc(filesize);
    if (!filedata) {
        printf("Error: Memory allocation failed\n");
        fclose(file);
        return;
    }
    
    // Ler ficheiro
    size_t bytes_read = fread(filedata, 1, filesize, file);
    fclose(file);
    
    if (bytes_read != (size_t)filesize) {
        printf("Error: Failed to read complete file\n");
        free(filedata);
        return;
    }
    
    const char *fname = strrchr(event_fname, '/');
    if (fname) {
        fname++;
    } else {
        fname = event_fname;
    }
    
    if (strlen(fname) > 24) {
        printf("Error: Filename too long (max 24 characters)\n");
        free(filedata);
        return;
    }
    
    // Conectar ao servidor TCP
    int tcp_socket = tcp_connect_to_server();
    if (tcp_socket == -1) {
        free(filedata);
        return;
    }
    
    
    // Construir header SEM espaço extra no final
    // Formato: "CRE UID password name date time attendance Fname Fsize " + filedata + "\n"
    char header[256];
    int header_len = snprintf(header, sizeof(header),
                              "%s %s %s %s %s %s %d %s %ld ",
                              CMD_CREATE, client_state.logged_uid, client_state.logged_password,
                              name, event_date, event_time, num_attendees, fname, filesize);
    
    if (header_len < 0 || header_len >= (int)sizeof(header)) {
        printf("Error: Command header too long\n");
        close(tcp_socket);
        free(filedata);
        return;
    }
    
    // Alocar buffer para mensagem completa (header + filedata + newline)
    size_t total_size = header_len + filesize + 1;
    char *full_message = malloc(total_size);
    if (!full_message) {
        printf("Error: Memory allocation failed for message\n");
        close(tcp_socket);
        free(filedata);
        return;
    }
    
    // Montar mensagem completa
    memcpy(full_message, header, header_len);
    memcpy(full_message + header_len, filedata, filesize);
    full_message[header_len + filesize] = '\n';

    size_t total_sent = 0;

    while (total_sent < total_size) {
        
        ssize_t n = write(tcp_socket, full_message + total_sent, total_size - total_sent);
        if (n <= 0) {
            perror("Error sending data");
            close(tcp_socket);
            free(filedata);
            free(full_message);
            return;
        }
        total_sent += n;
    }
    free(full_message);
    
    free(filedata);
    
    // Receber resposta: "RCE status [EID]\n"
    char response[64];
    ssize_t resp_len = read(tcp_socket, response, sizeof(response) - 1);
    close(tcp_socket);
    
    if (resp_len <= 0) {
        printf("Error: No response from server\n");
        return;
    }
    
    response[resp_len] = '\0';
    
    // Parse resposta
    char rsp_code[4], status[4], eid[4];
    int parsed = sscanf(response, "%3s %3s %3s", rsp_code, status, eid);
    
    if (parsed < 2) {
        printf("Error: Invalid response format\n");
        return;
    }
    
    // Processar status
    if (strcmp(status, STATUS_OK) == 0 && parsed == 3) {
        printf("Event created successfully!\n");
        printf("───────────────────────────────\n");
        printf("    Event ID: %s\n", eid);
        printf("    Event Name: %s\n", name);
        printf("    Event Date: %s %s\n", event_date, event_time);
        printf("    Number of Attendees: %d\n", num_attendees);
    } else if (strcmp(status, STATUS_NOK) == 0) {
        printf("Error: Failed to create event (database full?)\n");
    } else if (strcmp(status, STATUS_NLG) == 0) {
        printf("Error: User not logged in or not found\n");
    } else if (strcmp(status, STATUS_WRP) == 0) {
        printf("Error: Wrong password\n");
    } else if (strcmp(status, STATUS_ERR) == 0) {
        printf("Error: Invalid command format\n");
    } else {
        printf("Unknown response: %s\n", response);
    }
}

/**
 * Comando: close EID
 * Fecha um evento criado pelo utilizador logado
 */
void cmd_close_event(const char* eid_str) {
    // Verificar se está logado
    if (!client_state.is_logged_in) {
        printf("Error: You must be logged in to close events\n");
        return;
    }
    
    // Converter EID para inteiro
    int eid = atoi(eid_str);
    
    // Validar EID
    if (eid < 1 || eid > 999) {
        printf("Error: Invalid EID (must be between 1 and 999)\n");
        return;
    }
    
    // Conectar ao servidor TCP
    int tcp_socket = tcp_connect_to_server();
    if (tcp_socket == -1) {
        return;
    }
    
    // Construir comando: "CLS UID password EID\n"
    char command[128];
    int cmd_len = snprintf(command, sizeof(command),
                          "%s %s %s %03d\n",
                          CMD_CLOSE, client_state.logged_uid, 
                          client_state.logged_password, eid);
    
    if (cmd_len < 0 || cmd_len >= (int)sizeof(command)) {
        printf("Error: Command too long\n");
        close(tcp_socket);
        return;
    }
    
    // Enviar comando
    ssize_t sent = write(tcp_socket, command, cmd_len);
    if (sent != cmd_len) {
        perror("Error sending command");
        close(tcp_socket);
        return;
    }
    
    
    // Receber resposta: "RCL status\n"
    char response[64];
    ssize_t resp_len = read(tcp_socket, response, sizeof(response) - 1);
    close(tcp_socket);
    
    if (resp_len <= 0) {
        printf("Error: No response from server\n");
        return;
    }
    
    response[resp_len] = '\0';
    
    // Parse resposta
    char rsp_code[4], status[4];
    int parsed = sscanf(response, "%3s %3s", rsp_code, status);
    
    if (parsed < 2) {
        printf("Error: Invalid response format\n");
        return;
    }
    
    // Processar status
    if (strcmp(status, STATUS_OK) == 0) {
        printf("Event closed successfully\n");

    } else if (strcmp(status, STATUS_NOK) == 0) {
        printf("Error: User does not exist or wrong password\n");
    } else if (strcmp(status, STATUS_NLG) == 0) {
        printf("Error: User not logged in\n");
    } else if (strcmp(status, STATUS_NOE) == 0) {
        printf("Error: Event does not exist (EID=%03d)\n", eid);
    } else if (strcmp(status, STATUS_EOW) == 0) {
        printf("Error: You are not the owner of this event\n");
    } else if (strcmp(status, STATUS_SLD) == 0) {
        printf("Event is already sold-out (all seats reserved)\n");
    } else if (strcmp(status, STATUS_PST) == 0) {
        printf("Event date has already passed\n");
    } else if (strcmp(status, STATUS_CLO) == 0) {
        printf("Event has been closed\n");
    } else if (strcmp(status, STATUS_ERR) == 0) {
        printf("Error: Invalid command format\n");
    } else {
        printf("Unknown response: %s\n", response);
    }
}

void cmd_my_events() {
    
    char message[64];
    char response[1024];

    // Validar que utilizador está autenticado
    if (!client_state.is_logged_in) {
        printf("Error: User needs to be logged in to view their events\n");
        return;
    }

    // Construir mensagem: "LME UID password\n"
    snprintf(message, sizeof(message), "%s %s %s\n", CMD_MYEVENTS, client_state.logged_uid, client_state.logged_password);
    
    // Enviar e receber resposta
    if (!send_udp_receive_response(message, response, sizeof(response))) {
        printf("Error: Communication with server failed\n");
        return;
    }
    
    // Parse resposta: "RME status [EID state]* \n"
    char rsp_code[4], status[4];
    if (sscanf(response, "%3s %3s", rsp_code, status) != 2) {
        printf("Error: Invalid response format\n");
        return;
    }
    
    // Processar status
    if (strcmp(status, STATUS_OK) == 0) {
        // Parse da lista de eventos
        char *ptr = response + 7;  // Saltar "RME OK "
        int event_count = 0;
        
        printf("My Events:\n\n");
        printf("  EID    State\n");
        printf("───────────────────────────────────────────\n");
        
        int eid, state;
        while (sscanf(ptr, "%d %d", &eid, &state) == 2) {
            event_count++;
            
            const char *status_str;
            
            
            switch (state) {
                case 0:
                    status_str = "Past (ended)";
                    break;
                case 1:
                    status_str = "Active (accepting reservations)";
                    break;
                case 2:
                    status_str = "Sold out";
                    break;
                case 3:
                    status_str = "Closed";
                    break;
                default:
                    status_str = "Unknown";
                    break;
            }
            
            printf("  %03d    %s\n", eid, status_str);
            
            // Avançar para próximo par EID state
            while (*ptr && *ptr != ' ') ptr++;
            if (*ptr) ptr++;  // Saltar espaço
            
            // Procurar próximo espaço (após state)
            while (*ptr && *ptr != ' ' && *ptr != '\n') ptr++;
            if (*ptr == ' ') ptr++;
            
        }
        
        printf("───────────────────────────────────────────\n");
        printf("  Total: %d event(s)\n", event_count);
        
        
    } else if (strcmp(status, STATUS_NOK) == 0) {
        printf("You have not created any events yet.\n");
        
    } else if (strcmp(status, STATUS_NLG) == 0) {
        printf("Error: User not logged in\n");
        
    } else if (strcmp(status, STATUS_WRP) == 0) {
        printf("Error: Wrong password\n");
        
    } else if (strcmp(status, STATUS_ERR) == 0) {
        printf("Error: Invalid request format\n");
        
    } else {
        printf("Unknown response: %s\n", response);
    }


}

void cmd_list_events() {

    if (!client_state.is_logged_in) {
        printf("Error: User needs to be logged in to view all available events\n");
        return;
    }

    char message[16];
    snprintf(message, sizeof(message), "%s\n", CMD_LIST);

    int tcp_socket = tcp_connect_to_server();
    if (tcp_socket == -1) return;

    ssize_t sent = write(tcp_socket, message, strlen(message));
    if (sent != (ssize_t)strlen(message)) {
        perror("Error sending command");
        close(tcp_socket);
        return;
    }


    char recv_buf[128];      // Buffer de leitura do socket
    char tail[64] = "";     // Buffer auxiliar para dados TRUNCADOS
    size_t tail_len = 0;

    bool got_header = false;
    int event_count = 0;
    int iteration = 0;


    while (1) {
        iteration++;

        ssize_t n = read(tcp_socket, recv_buf, sizeof(recv_buf) - 1);
        if (n < 0) {
            perror("Error reading from server");
            break;
        } else if (n == 0) {
            break;
        }
        recv_buf[n] = '\0';

        // Criar buffer de trabalho: tail (dados anteriores) + recv_buf (novos dados)
        char work_buf[512];
        if (tail_len + (size_t)n >= sizeof(work_buf)) {
            fprintf(stderr, "Error: work_buf overflow\n");
            break;
        }
        
        memcpy(work_buf, tail, tail_len);
        memcpy(work_buf + tail_len, recv_buf, n);
        work_buf[tail_len + n] = '\0';
        size_t work_len = tail_len + n;
        

        size_t pos = 0;

        if (!got_header) {
            if (work_len < 7) {
                memcpy(tail, work_buf, work_len);
                tail_len = work_len;
                continue;
            }

            char rsp[4], status[4];
            int consumed = 0;
            if (sscanf(work_buf, "%3s %3s %n", rsp, status, &consumed) != 2) {
                fprintf(stderr, "Error: invalid LIST header\n");
                break;
            }


            if (strcmp(rsp, RSP_LIST) != 0 || strcmp(status, STATUS_OK) != 0) {
                if (strcmp(status, STATUS_NOK) == 0) {
                    printf("No events available.\n");
                } else {
                    fprintf(stderr, "Error: Unexpected response\n");
                }
                close(tcp_socket);
                return;
            }

            got_header = true;
            pos = (size_t)consumed;
        }
        if (iteration == 1) {
            printf("  Event ID      Event Name          Event State      Event Date\n");
            printf("─────────────────────────────────────────────────────────────────\n");
        }

        int events_this_iter = 0;
        while (pos < work_len) {
            // Verificar se encontramos o '\n' final
            if (work_buf[pos] == '\n') {
                close(tcp_socket);
                printf("Total events listed: %d\n", event_count);
                return;
            }

            int eid, state, consumed = 0;
            char name[EVENT_NAME_LEN + 1];
            char date[DATE_STR_LEN + 1];
            char time[6];

            int rc = sscanf(work_buf + pos, "%d %10s %d %10s %5s %n",
                           &eid, name, &state, date, time, &consumed);


            if (rc == 5 && consumed > 0 && strlen(time) == 5 && time[2] == ':') {
                // Evento COMPLETO!
                event_count++;
                events_this_iter++;

                const char *status_str;
                if (state == 0) status_str = "Past";
                else if (state == 1) status_str = "Active";
                else if (state == 2) status_str = "Sold Out";
                else if (state == 3) status_str = "Closed";
                else status_str = "Unknown";

                printf("    %03d          %s             %s      %s %s\n",
                       eid, name, status_str, date, time);

                pos += (size_t)consumed;
            } else {
                break;
            }
        }

        /* 3) GUARDAR RESTO NO TAIL */
        tail_len = work_len - pos;
        if (tail_len > 0) {
            if (tail_len >= sizeof(tail)) {
                fprintf(stderr, "Error: tail overflow (%zu bytes)\n", tail_len);
                break;
            }
            memcpy(tail, work_buf + pos, tail_len);
            tail[tail_len] = '\0';
        }
    }

    close(tcp_socket);
    printf("─────────────────────────────────────────────────────────────────\n");
    printf("Total events listed: %d\n", event_count);
}

void cmd_change_password(const char* old_pass, const char* new_pass) {  
    // A implementar
    char command[128];
    char response[64];

    //Validar formato das passwords
    if (strlen(old_pass) != PASSWORD_LEN || strlen(new_pass) != PASSWORD_LEN) {
        printf("Error: Passwords must be exactly %d alphanumeric characters\n", PASSWORD_LEN);
        return;
    }

    int cmd_len = snprintf(command, sizeof(command),
                           "%s %s %s %s\n",
                           CMD_CHANGE_PASS, client_state.logged_uid,
                           old_pass, new_pass);

    int tcp_socket = tcp_connect_to_server();
    if (tcp_socket == -1) {
        return;
    }

    ssize_t sent = write(tcp_socket, command, cmd_len);
    if (sent != cmd_len) {
        perror("Error sending command");
        close(tcp_socket);
        return;
    }

    ssize_t resp_len = read(tcp_socket, response, sizeof(response) - 1);
    close(tcp_socket);

    if (resp_len <= 0) {
        printf("Error: No response from server\n");
        return;
    }

    response[resp_len] = '\0';
    char rsp_code[4], status[4];
    int parsed = sscanf(response, "%3s %3s", rsp_code, status);
    if (parsed < 2) {
        printf("Error: Invalid response format\n");
        return;
    }

    if (strcmp(status, STATUS_OK) == 0) {
        printf("Password changed successfully!\n");
        strcpy(client_state.logged_password, new_pass);
        
    } else if (strcmp(status, STATUS_NLG) == 0) {
        printf("Error: User not logged in\n");
        
    } else if (strcmp(status, STATUS_NOK) == 0) {
        printf("Error: Wrong old password\n");

    } else if (strcmp(status, STATUS_NID) == 0) {
        printf("Error: User does not exist\n");
        
    } else if (strcmp(status, STATUS_ERR) == 0) {
        printf("Error: Invalid command format\n");
        
    } else {
        printf("Unknown response: %s\n", response);
    }
}

void cmd_show_event(const char* eid_str) {

    if (!client_state.is_logged_in) {
        printf("Error: User needs to be logged in to view event details\n");
        return;
    }

    if (strlen(eid_str) > 3) {
        printf("Error: Invalid EID (must be between 1 and 999)\n");
        return;
    }

    int eid = atoi(eid_str);
    if (eid < 1 || eid > 999) {
        printf("Error: Invalid EID (must be between 1 and 999)\n");
        return;
    }

    // Alocar buffer grande no HEAP (não na stack!)
    char *response = malloc(10485760); // 10 MB
    if (!response) {
        printf("Error: Memory allocation failed\n");
        return;
    }
    
    char message[64];
    size_t total_received = 0;

    snprintf(message, sizeof(message), "%s %s\n",
             CMD_SHOW, eid_str);

    int tcp_socket = tcp_connect_to_server();
    if (tcp_socket == -1) {
        free(response);
        return;
    }

    ssize_t sent = write(tcp_socket, message, strlen(message));
    if (sent != (ssize_t)strlen(message)) {
        perror("Error sending command");
        close(tcp_socket);
        free(response);
        return;
    }
    
    bool message_complete = false;
    while (!message_complete && total_received < 10485760 - 1) {
        
        ssize_t n = read(tcp_socket, response + total_received, 10485760 - 1 - total_received);
        
        if (n <= 0) {
            if (n == 0) {

            } else {
                perror("Error reading response");
            }
            close(tcp_socket);
            if (total_received == 0) {
                printf("Error: No response from server\n");
                free(response);
                return;
            }
            break;
        }
        
        total_received += n;
        
        // Verificar se recebemos o '\n' final
        if (response[total_received - 1] == '\n') {
            message_complete = true;
        }
    }

    close(tcp_socket);
    response[total_received] = '\0';

    char rsp_code[4], status[4];
    if (sscanf(response, "%3s %3s", rsp_code, status) != 2) {
        printf("Error: Invalid response format\n");
        free(response);
        return;
    }

    if (strcmp(status, STATUS_OK) == 0) {
        char *ptr = response + 7;  // Saltar "RSE OK "
        
        char uid[7], event_name[EVENT_NAME_LEN + 1], fname[25];
        char event_date[DATE_STR_LEN + 1], event_time[6];;
        int num_attendees, num_reserved;
        long fsize;
        
        // Parse dos 8 campos de texto
        int parsed = sscanf(ptr, "%6s %10s %10s %5s %d %d %24s %ld",
                            uid, event_name, event_date, event_time,
                            &num_attendees, &num_reserved, fname, &fsize);
        
        if (parsed != 8) {
            printf("Error: Invalid event data format (parsed=%d)\n", parsed);
            free(response);
            return;
        }
    
        char *fdata_ptr = ptr;  // ptr já aponta depois de "RSE OK "
        
        // Saltar os 8 campos que já lemos
        int fields_to_skip = 8;
        while (fields_to_skip > 0 && fdata_ptr < response + total_received) {
            // Saltar espaços
            while (*fdata_ptr == ' ' && fdata_ptr < response + total_received) {
                fdata_ptr++;
            }
            // Saltar o campo
            while (*fdata_ptr != ' ' && fdata_ptr < response + total_received) {
                fdata_ptr++;
            }
            fields_to_skip--;
        }
        
        // Agora saltar o último espaço antes do Fdata
        while (*fdata_ptr == ' ' && fdata_ptr < response + total_received) {
            fdata_ptr++;
        }
        
        // Calcular tamanho real do Fdata recebido
        long fdata_received = (response + total_received) - fdata_ptr;
        if (fdata_received > 0 && response[total_received - 1] == '\n') {
            fdata_received--; // Não contar o \n final
        }
        
        if (fdata_received < fsize) {
            printf("Error: Incomplete file (expected=%ld, got=%ld)\n", 
                   fsize, fdata_received);
            free(response);
            return;
        }
        
        // FASE 3: Guardar ficheiro no disco
        FILE *f = fopen(fname, "wb");
        if (!f) {
            printf("Error: Cannot create file '%s'\n", fname);
            free(response);
            return;
        }
        
        size_t written = fwrite(fdata_ptr, 1, fsize, f);
        fclose(f);
        
        if (written != (size_t)fsize) {
            printf("Error: Failed to write complete file\n");
            free(response);
            return;
        }
        
        // Mostrar informações
        printf("Event %s Details:\n", eid_str);
        printf("────────────────────────────────────────────────────────────\n");
        printf("  Name:             %-35s\n", event_name);
        printf("  Date & Time:      %s %s%-20s\n", event_date, event_time, "");
        printf("  Host:             %-35s\n", uid);
        printf("────────────────────────────────────────────────────────────\n");
        printf("  Total Seats:      %-28d\n", num_attendees);
        printf("  Reserved:         %-28d\n", num_reserved);
        printf("  Available:        %-28d\n", num_attendees - num_reserved);
        printf("────────────────────────────────────────────────────────────\n");
        printf("  Description file:             %-35s\n", fname);
        printf("  File Size:             %-25ld bytes\n", fsize);

        // Determinar status (sold-out ou passado)
        char full_datetime[20];
        snprintf(full_datetime, sizeof(full_datetime), "%s %s", event_date, event_time);
        if (num_reserved >= num_attendees) {
            printf("────────────────────────────────────────────────────────────\n");
            printf("  This event is sold-out!\n");
        } else if (is_date_before_now(full_datetime)) {
            printf("────────────────────────────────────────────────────────────\n");
            printf("  This event already happened in the past!\n");
        }
        printf("\n");
        
    } else if (strcmp(status, STATUS_NOK) == 0) {
        printf("Error: Event does not exist\n");
    } else {
        printf("Unknown response: %s\n", response);
    }
    
    free(response);
}

void cmd_reserve(const char* eid_str, int num_seats) {
    
    char command[128];
    char response[64];
    int available_seats;

    if (!client_state.is_logged_in) {
        printf("Error: User needs to be logged in to reserve seats\n");
        return;
    }

    int eid = atoi(eid_str);
    if (eid < 1 || eid > 999) {
        printf("Error: Invalid EID (must be between 1 and 999)\n");
        return;
    }

    if (num_seats < 1 || num_seats > 999) {
        printf("Error: Number of seats must be between 1 and 999\n");
        return;
    }

    int msg_len = snprintf(command, sizeof(command), "%s %s %s %s %d\n",
                           CMD_RESERVE, client_state.logged_uid, client_state.logged_password, eid_str, num_seats);

    if (msg_len < 0 || msg_len >= (int)sizeof(command)) {
        printf("Error: Command too long\n");
        return;
    }
    
    int tcp_socket = tcp_connect_to_server();
    if (tcp_socket == -1) {
        return;
    }

    ssize_t sent = write(tcp_socket, command, msg_len);
    if (sent != msg_len) {
        perror("Error sending command");
        close(tcp_socket);
        return;
    }

    ssize_t resp_len = read(tcp_socket, response, sizeof(response) - 1);
    close(tcp_socket);
    
    if (resp_len <= 0) {
        printf("Error: No response from server\n");
        return;
    }
    
    response[resp_len] = '\0';

    char rsp_code[4], status[4];
    int parsed = sscanf(response, "%3s %3s", rsp_code, status);

    if (parsed < 2 || strcmp(rsp_code, RSP_RESERVE) != 0) {
        printf("Error: Invalid response format\n");
        return;
    }

    if (strcmp(status, STATUS_ACC) == 0) {
        
        printf("Event reserved successfully!\n");
        printf("────────────────────────────────────────────────────────────\n");
        if (num_seats == 1)  {            
            printf("  %d seat reserved for event %s\n", num_seats, eid_str);
        } else {
            printf("  %d seat(s) reserved for event %s\n", num_seats, eid_str);
        }
        
    } else if (strcmp(status, STATUS_REJ) == 0) {
        
        if (sscanf(response, "%3s %3s %d", rsp_code, status, &available_seats) == 3) {
            printf("Reservation rejected: not enough seats available\n");
            printf("────────────────────────────────────────────────────────────\n");
            printf("  Requested: %d seats\n", num_seats);
            printf("  Available: %d seats\n", available_seats);
        
        } else {
            printf("Error: Reservation rejected (could not parse available seats)\n");
        }
        
    } else if (strcmp(status, STATUS_NLG) == 0) {
        printf("Error: User not logged in\n");
        
    } else if (strcmp(status, STATUS_NOK) == 0) {
        printf("Error: Event %s is not active\n", eid_str);

    } else if (strcmp(status, STATUS_CLS) == 0) {
        printf("Error: Event %s is closed\n", eid_str);
        
    } else if (strcmp(status, STATUS_SLD) == 0) {
        printf("Error: Event %s is sold out\n", eid_str);
    
    } else if (strcmp(status, STATUS_PST) == 0) {
        printf("Error: Event date has already passed\n");
    
    } else if (strcmp(status, STATUS_WRP) == 0) {
        printf("Error: Wrong password\n");
    
    } else if (strcmp(status, STATUS_ERR) == 0) {
        printf("Error: Invalid command format\n");

    } else {
        printf("Unknown response: %s\n", response);
    }


}

void cmd_my_reservations() {
    
    char message[64];
    char response[1024];

    // Validar que utilizador está autenticado
    if (!client_state.is_logged_in) {
        printf("Error: User needs to be logged in to view their reservations\n");
        return;
    }

    // Construir mensagem: "LMR UID password\n"
    snprintf(message, sizeof(message), "%s %s %s\n", CMD_MY_RESERVATIONS, client_state.logged_uid, client_state.logged_password);

    if (!send_udp_receive_response(message, response, sizeof(response))) {
        printf("Error: Communication with server failed\n");
        return;
    }

    // Parse resposta: "RMR status [EID date value]* \n"
    char rsp_code[4], status[4];
    if (sscanf(response, "%3s %3s", rsp_code, status) != 2) {
        printf("Error: Invalid response format\n");
        return;
    }

    // Processar status
    if (strcmp(status, STATUS_OK) == 0) {

        // Parse da lista de reservas
        char *ptr = response + 7;  // Saltar "RMR OK "
        int reservation_count = 0;

        printf("  My Reservations\n");
        printf("────────────────────────────────────────────────────────────\n");
        printf("  Event ID         Date       Time       Number of Seats\n");
        printf("────────────────────────────────────────────────────────────\n");

        int eid;
        char date[DATE_STR_LEN + 1];
        char time[TIME_STR_LEN + 4]; // espaço para formato HH:MM:SS
        int num_seats;

        while (sscanf(ptr, "%d %10s %8s %d", &eid, date, time, &num_seats) == 4) {
            reservation_count++;

            printf("    %03d        %s   %s         %d \n", eid, date, time, num_seats);
            
            // Avançar para próximo conjunto EID date time value
            int spaces_count = 0;
            while (spaces_count < 4 && *ptr) {
                if (*ptr == ' ' || *ptr == '\n') {
                    spaces_count++;
                }
                ptr++;
            }
            
            
        }

        printf("────────────────────────────────────────────────────────────\n");
        printf("  Total: %d reservation(s)\n", reservation_count);

    } else if (strcmp(status, STATUS_NOK) == 0) {
        printf("You have no reservations yet.\n");

    } else if (strcmp(status, STATUS_NLG) == 0) {
        printf("Error: User not logged in\n");

    } else if (strcmp(status, STATUS_WRP) == 0) {
        printf("Error: Wrong password\n");

    } else if (strcmp(status, STATUS_ERR) == 0) {
        printf("Error: Invalid request format\n");

    } else {
        printf("Unknown response: %s\n", response);
    }

}


CommandType parse_command_type(const char* command) {

    if (strcmp(command, "login") == 0) return CMD_TYPE_LOGIN;
    if (strcmp(command, "create") == 0) return CMD_TYPE_CREATE;
    if (strcmp(command, "logout") == 0) return CMD_TYPE_LOGOUT;
    if (strcmp(command, "close") == 0) return CMD_TYPE_CLOSE;
    if (strcmp(command, "myevents") == 0 || strcmp(command, "mye") == 0) return CMD_TYPE_MY_EVENTS;
    if (strcmp(command, "list") == 0) return CMD_TYPE_LIST;
    if (strcmp(command, "reserve") == 0) return CMD_TYPE_RESERVE;
    if (strcmp(command, "unregister") == 0) return CMD_TYPE_UNREGISTER;
    if (strcmp(command, "changePass") == 0) return CMD_TYPE_CHANGE_PASSWORD;
    if (strcmp(command, "show") ==0) return CMD_TYPE_SHOW;
    if (strcmp(command, "myreservations") == 0 || strcmp(command, "myr") == 0) return CMD_TYPE_MY_RESERVATIONS;
    if (strcmp(command, "help") == 0) return CMD_TYPE_HELP;
    if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) return CMD_TYPE_EXIT;
    return CMD_TYPE_UNKNOWN;
}


int main(int argc, char *argv[]) {

    char *server_ip = "localhost";
    char *server_port = "58084";
    char input[INPUT_BUFFER_SIZE];
    char command[32], arg1[32], arg2[32];
    
    // Parse argumentos da linha de comandos
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            server_ip = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            server_port = argv[++i];
        }
    }
    
    // Inicializar conexão UDP
    if (!init_udp_connection(server_ip, server_port)) {
        fprintf(stderr, "Failed to initialize connection\n");
        return 1;
    }
    
    printf("User application started. Type 'help' for available commands.\n");
    
    // Main loop
    while (1) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        
        input[strcspn(input, "\n")] = '\0';
        
        
        char arg3[32], arg4[32], arg5[256];
        int parsed = sscanf(input, "%31s %31s %31s %31s %31s %255s", command, arg1, arg2, arg3, arg4, arg5);
        
        if (parsed == 0) {
            continue; // Linha vazia
        }
        
        // Processar comandos com switch
        CommandType cmd_type = parse_command_type(command);
        
        switch (cmd_type) {
            case CMD_TYPE_LOGIN:
                if (parsed == 3) {
                    cmd_login(arg1, arg2);
                } else {
                    printf("Usage: login UID password\n");
                }
                break;
            case CMD_TYPE_LOGOUT:
                if (parsed == 1) {
                    cmd_logout();
                } else {
                    printf("Usage: logout\n");
                }
                break;
            case CMD_TYPE_UNREGISTER:
                if (parsed == 1) {
                    cmd_unregister();
                } else {
                    printf("Usage: unregister\n");
                }
                break;
            case CMD_TYPE_CHANGE_PASSWORD:
                if (parsed == 3) {
                    cmd_change_password(arg1, arg2);
                } else {
                    printf("Usage: changePass old_password new_password\n");
                }
                break;
            case CMD_TYPE_CREATE:
                if (parsed == 6) {
                    int num_attendees = atoi(arg5);
                    cmd_create_event(arg1, arg2, arg3, arg4, num_attendees);
                } else {
                    printf("Usage: create name event_fname event_date event_time num_attendees\n");
                }
                break;
            case CMD_TYPE_CLOSE:
                if (parsed == 2) {
                    cmd_close_event(arg1);
                } else {
                    printf("Usage: close EID\n");
                }
                break;
            case CMD_TYPE_MY_EVENTS:
                if (parsed == 1) {
                    cmd_my_events();
                }
                else {
                    printf("Usage: myevents\n");
                }
                break;
            case CMD_TYPE_LIST:
                if (parsed  ==  1) {
                    cmd_list_events();
                }
                else {
                    printf("Usage: list\n");
                }
                break;
            case CMD_TYPE_SHOW:
                if (parsed == 2) {
                    cmd_show_event(arg1);
                } else {
                    printf("Usage: show EID\n");
                }
                break;
            case CMD_TYPE_RESERVE:
                if (parsed == 3) {
                    cmd_reserve(arg1, atoi(arg2));
                } else {
                    printf("Usage: reserve EID num_seats\n");
                }
                break;
            case CMD_TYPE_MY_RESERVATIONS:
                if (parsed == 1) {
                    cmd_my_reservations();
                } else {
                    printf("Usage: myreservations\n");
                }
                break;
            case CMD_TYPE_HELP:
                show_help();
                break;
                
            case CMD_TYPE_EXIT:
                if (client_state.is_logged_in) {
                    printf("Warning: You are still logged in. Logging out automatically...\n");
                    cmd_logout();
                }
                printf("Exiting...\n");
                goto cleanup_and_exit;
                
            case CMD_TYPE_UNKNOWN:
            default:
                printf("Unknown command '%s'. Type 'help' for available commands.\n", command);
                break;
        }
    }
    
cleanup_and_exit:    // Cleanup
    if (client_state.udp_socket != -1) {
        close(client_state.udp_socket);
    }
    if (client_state.server_addr != NULL) {
        freeaddrinfo(client_state.server_addr);
    }
    
    return 0;
}
