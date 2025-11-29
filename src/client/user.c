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

// Estado global do cliente
static struct {
    int udp_socket;
    struct addrinfo *server_addr;
    char logged_uid[7];  // UID do utilizador logado (ou vazio)
    char logged_password[9];    // Password do utilizador logado (ou vazia)
    bool is_logged_in;
} client_state = {-1, NULL, "", "", false};

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

// Enum created in protocol.h 


CommandType parse_command_type(const char* command) {
    if (strcmp(command, "login") == 0) return CMD_TYPE_LOGIN;
    if (strcmp(command, "logout") == 0) return CMD_TYPE_LOGOUT;
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
        int parsed = sscanf(input, "%31s %31s %31s", command, arg1, arg2);
        
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
            case CMD_TYPE_HELP:
                show_help();
                break;
                
            case CMD_TYPE_EXIT:
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
