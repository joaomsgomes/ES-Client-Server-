#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "../include/protocol.h"
#include "../include/utils.h"

#define INPUT_BUFFER_SIZE 256
#define PASSWORD_LEN 8
#define UID_LEN 6

// Estado global do cliente
static struct {
    int udp_socket;
    struct addrinfo *server_addr;
    char logged_uid[7];
    char logged_password[9];
    bool is_logged_in;
} client_state = {-1, NULL, "000000", "00000000", false};

/**
 * Inicializa conexão UDP com o servidor
 */
bool init_udp_connection(const char* server_ip, const char* server_port) {
    struct addrinfo hints;
    int errcode;
    
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
        perror("Error receiving UDP response");
        return false;
    }
    
    response[n] = '\0';
    return true;
}

/**
 * Comando: login UID password
 */
void cmd_login(const char* uid, const char* password) {
    char message[64];
    char response[64];
    
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
        printf("Login successful\n");
        
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
        printf("Successful logout\n");
        
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
        printf("Error: Unknown user\n");
        
    } else if (strcmp(status, STATUS_WRP) == 0) {
        printf("Error: Wrong password\n");

    } else if (strcmp(status, STATUS_ERR) == 0) {
        printf("Error: Invalid request format\n");
        
    } else {
        printf("Unknown response: %s\n", response);
    }
}


void cmd_create(const char* name, const char* event_fname, const char* event_date, int num_attendees) {
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
    
    if (!validate_date(event_date)) {
        printf("Event date: %s\n", event_date);
        printf("Error: Invalid date format (use dd-mm-yyyy)\n");
        return;
    }
    
    if (num_attendees < 10 || num_attendees > 999) {
        printf("Error: Attendance must be between 10 and 999\n");
        return;
    }
    
    // Abrir e ler ficheiro (deve existir na mesma pasta)
    FILE *file = fopen(event_fname, "rb");
    if (!file) {
        printf("Error: Cannot open file '%s' (must exist in current folder)\n", event_fname);
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
    
    // Extrair apenas o nome do ficheiro (sem path)
    const char *fname = strrchr(event_fname, '/');
    if (fname) {
        fname++; // Avançar após '/'
    } else {
        fname = event_fname;
    }
    
    if (strlen(fname) > 24) {
        printf("Error: Filename too long (max 24 characters)\n");
        free(filedata);
        return;
    }
    
    printf("Creating event '%s' with file '%s' (%ld bytes)...\n", name, fname, filesize);
    
    // Conectar ao servidor TCP (porta 58001)
    struct addrinfo hints, *tcp_res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    int errcode = getaddrinfo("localhost", "58001", &hints, &tcp_res);
    if (errcode != 0) {
        fprintf(stderr, "Error: getaddrinfo TCP: %s\n", gai_strerror(errcode));
        free(filedata);
        return;
    }
    
    int tcp_socket = socket(tcp_res->ai_family, tcp_res->ai_socktype, tcp_res->ai_protocol);
    if (tcp_socket == -1) {
        perror("Error creating TCP socket");
        freeaddrinfo(tcp_res);
        free(filedata);
        return;
    }
    
    if (connect(tcp_socket, tcp_res->ai_addr, tcp_res->ai_addrlen) == -1) {
        perror("Error connecting to TCP server");
        close(tcp_socket);
        freeaddrinfo(tcp_res);
        free(filedata);
        return;
    }
    
    freeaddrinfo(tcp_res);
    
    // Construir comando: "CRE UID password name date attendance Fname Fsize Fdata\n"
    char header[256];
    int header_len = snprintf(header, sizeof(header),
                              "%s %s %s %s %s %d %s %ld ",
                              CMD_CREATE, client_state.logged_uid, client_state.logged_password,
                              name, event_date, num_attendees, fname, filesize);
    
    if (header_len < 0 || header_len >= (int)sizeof(header)) {
        printf("Error: Command header too long\n");
        close(tcp_socket);
        free(filedata);
        return;
    }
    
    // Enviar header
    ssize_t sent = send(tcp_socket, header, header_len, 0);
    if (sent != header_len) {
        perror("Error sending command header");
        close(tcp_socket);
        free(filedata);
        return;
    }
    
    // Enviar filedata (dados binários)
    size_t total_sent = 0;
    while (total_sent < (size_t)filesize) {
        ssize_t n = send(tcp_socket, filedata + total_sent, filesize - total_sent, 0);
        if (n <= 0) {
            perror("Error sending file data");
            close(tcp_socket);
            free(filedata);
            return;
        }
        total_sent += n;
    }
    
    // Enviar newline final
    if (send(tcp_socket, "\n", 1, 0) != 1) {
        perror("Error sending final newline");
        close(tcp_socket);
        free(filedata);
        return;
    }
    
    free(filedata);
    
    printf("Command sent successfully, waiting for response...\n");
    
    // Receber resposta: "RCE status [EID]\n"
    char response[64];
    ssize_t resp_len = recv(tcp_socket, response, sizeof(response) - 1, 0);
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
        printf("✓ Event created successfully!\n");
        printf("  EID: %s\n", eid);
        printf("  Name: %s\n", name);
        printf("  Date: %s\n", event_date);
        printf("  Attendees: %d\n", num_attendees);
    } else if (strcmp(status, STATUS_NOK) == 0) {
        printf("✗ Error: Failed to create event (database full?)\n");
    } else if (strcmp(status, STATUS_NLG) == 0) {
        printf("✗ Error: User not logged in or not found\n");
    } else if (strcmp(status, STATUS_WRP) == 0) {
        printf("✗ Error: Wrong password\n");
    } else if (strcmp(status, STATUS_ERR) == 0) {
        printf("✗ Error: Invalid command format\n");
    } else {
        printf("Unknown response: %s\n", response);
    }
}

/**
 * Comando: close EID
 * Fecha um evento criado pelo utilizador logado
 */
void cmd_close(const char* eid_str) {
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
    
    printf("Closing event EID=%03d...\n", eid);
    
    // Conectar ao servidor TCP (porta 58001)
    struct addrinfo hints, *tcp_res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    int errcode = getaddrinfo("localhost", "58001", &hints, &tcp_res);
    if (errcode != 0) {
        fprintf(stderr, "Error: getaddrinfo TCP: %s\n", gai_strerror(errcode));
        return;
    }
    
    int tcp_socket = socket(tcp_res->ai_family, tcp_res->ai_socktype, tcp_res->ai_protocol);
    if (tcp_socket == -1) {
        perror("Error creating TCP socket");
        freeaddrinfo(tcp_res);
        return;
    }
    
    if (connect(tcp_socket, tcp_res->ai_addr, tcp_res->ai_addrlen) == -1) {
        perror("Error connecting to TCP server");
        close(tcp_socket);
        freeaddrinfo(tcp_res);
        return;
    }
    
    freeaddrinfo(tcp_res);
    
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
    ssize_t sent = send(tcp_socket, command, cmd_len, 0);
    if (sent != cmd_len) {
        perror("Error sending command");
        close(tcp_socket);
        return;
    }
    
    printf("Command sent, waiting for response...\n");
    
    // Receber resposta: "RCL status\n"
    char response[64];
    ssize_t resp_len = recv(tcp_socket, response, sizeof(response) - 1, 0);
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
        printf("✓ Event closed successfully!\n");
        printf("  EID %03d is now closed for new reservations\n", eid);
    } else if (strcmp(status, STATUS_NOK) == 0) {
        printf("✗ Error: User does not exist or wrong password\n");
    } else if (strcmp(status, STATUS_NLG) == 0) {
        printf("✗ Error: User not logged in\n");
    } else if (strcmp(status, STATUS_NOE) == 0) {
        printf("✗ Error: Event does not exist (EID=%03d)\n", eid);
    } else if (strcmp(status, STATUS_EOW) == 0) {
        printf("✗ Error: You are not the owner of this event\n");
    } else if (strcmp(status, STATUS_SLD) == 0) {
        printf("ℹ Event is already sold-out (all seats reserved)\n");
    } else if (strcmp(status, STATUS_PST) == 0) {
        printf("ℹ Event date has already passed\n");
    } else if (strcmp(status, STATUS_CLO) == 0) {
        printf("ℹ Event has been closed\n");
    } else if (strcmp(status, STATUS_ERR) == 0) {
        printf("✗ Error: Invalid command format\n");
    } else {
        printf("Unknown response: %s\n", response);
    }
}

void cmd_mye() {
    
    char message[64];
    char response[64];

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
        
        printf("\n═══════════════════════════════════════════════════════\n");
        printf("  Your Events\n");
        printf("═══════════════════════════════════════════════════════\n");
        printf("  EID    Status\n");
        printf("───────────────────────────────────────────────────────\n");
        
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
                    status_str = "Closed by you";
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
        
        printf("───────────────────────────────────────────────────────\n");
        printf("  Total: %d event(s)\n", event_count);
        printf("═══════════════════════════════════════════════════════\n\n");
        
    } else if (strcmp(status, STATUS_NOK) == 0) {
        printf("You have not created any events yet\n");
        
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
    if (strcmp(command, "myevents") == 0 || strcmp(command, "mye") == 0) return CMD_TYPE_MYEVENTS;
    if (strcmp(command, "unregister") == 0) return CMD_TYPE_UNREGISTER;
    if (strcmp(command, "help") == 0) return CMD_TYPE_HELP;
    if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) return CMD_TYPE_EXIT;
    return CMD_TYPE_UNKNOWN;
}


int main(int argc, char *argv[]) {

    char *server_ip = "localhost";
    char *server_port = "58000";
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
        
        // Remove newline
        input[strcspn(input, "\n")] = '\0';
        
        // Parse comando
        char arg3[32], arg4[256];
        int parsed = sscanf(input, "%31s %31s %31s %31s %255s", command, arg1, arg2, arg3, arg4);
        
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
            case CMD_TYPE_CREATE:
                if (parsed == 5) {
                    int num_attendees = atoi(arg4);
                    cmd_create(arg1, arg2, arg3, num_attendees);
                } else {
                    printf("Usage: create name event_fname event_date num_attendees\n");
                    printf("Example: create Concert poster.jpg 31-12-2025 500\n");
                }
                break;
            case CMD_TYPE_CLOSE:
                if (parsed == 2) {
                    cmd_close(arg1);
                } else {
                    printf("Usage: close EID\n");
                }
                break;
            case CMD_TYPE_MYEVENTS:
                if (parsed == 1) {
                    cmd_mye();
                }
                else {
                    printf("Usage: myevents\n");
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
