#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../../include/es_server.h"
#include "../../include/protocol.h"
#include "../../include/user_management.h"
#include "../../include/utils.h"

/**
 * Processa comando LOGIN recebido via UDP
 * 
 * Mensagem recebida: "LIN UID password\n"
 * 
 * Respostas possíveis:
 *   "RLI OK\n"  - Login bem-sucedido (utilizador já existia)
 *   "RLI REG\n" - Registo + login bem-sucedido (novo utilizador)
 *   "RLI NOK\n" - Password incorreta
 *   "RLI ERR\n" - Erro de formato/sintaxe
 */
void handle_login(int sockfd, char* message, struct sockaddr_in* client_addr, socklen_t addrlen) {
    
    char cmd[4], uid[UID_LEN + 1], password[PASSWORD_LEN + 1];
    char response[32];
    
    // Parse: "LIN UID password\n"
    int parsed = sscanf(message, "%3s %6s %8s", cmd, uid, password);
    
    if (parsed != 3) {
        // Formato inválido
        snprintf(response, sizeof(response), "%s %s\n", RSP_LOGIN, STATUS_ERR);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] LOGIN: Invalid format\n");
        return;
    }
    
    // Validar formato de UID e password
    if (!validate_uid(uid) || !validate_password(password)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_LOGIN, STATUS_ERR);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] LOGIN: Invalid UID or password format (UID=%s)\n", uid);
        return;
    }
    
    // Verificar se utilizador existe e autenticar
    int auth_result = authenticate_user(uid, password);
    
    if (auth_result == 1) {
        // Utilizador existe e password correta
        snprintf(response, sizeof(response), "%s %s\n", RSP_LOGIN, STATUS_OK);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] LOGIN: User %s logged in successfully\n", uid);
        
    } else if (auth_result == 0) {
        // Utilizador não existe - registar novo utilizador
        if (register_user(uid, password)) {
            snprintf(response, sizeof(response), "%s %s\n", RSP_LOGIN, STATUS_REG);
            sendto(sockfd, response, strlen(response), 0, 
                   (struct sockaddr*)client_addr, addrlen);
            printf("[UDP] LOGIN: New user %s registered successfully\n", uid);
        } else {
            snprintf(response, sizeof(response), "%s %s\n", RSP_LOGIN, STATUS_ERR);
            sendto(sockfd, response, strlen(response), 0, 
                   (struct sockaddr*)client_addr, addrlen);
            printf("[UDP] LOGIN: Failed to register user %s\n", uid);
        }
        
    } else {
        // auth_result == -1: Password incorreta
        snprintf(response, sizeof(response), "%s %s\n", RSP_LOGIN, STATUS_NOK);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] LOGIN: Wrong password for user %s\n", uid);
    }
}
