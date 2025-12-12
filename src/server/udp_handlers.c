#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <dirent.h>
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
        login_user(uid);
        snprintf(response, sizeof(response), "%s %s\n", RSP_LOGIN, STATUS_OK);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] LOGIN: User %s logged in successfully\n", uid);
        
    } else if (auth_result == 0) {
        // Utilizador não existe - registar novo utilizador
        if (register_user(uid, password)) {
            login_user(uid);
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

void handle_logout(int sockfd, char* message, struct sockaddr_in* client_addr, socklen_t addrlen) {
    
    char cmd[4], uid[UID_LEN + 1], password[PASSWORD_LEN + 1];
    char response[32];
    
    // Parse: "LOU UID password\n"
    int parsed = sscanf(message, "%3s %6s %8s", cmd, uid, password);

    if (parsed != 3) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_LOGOUT, STATUS_ERR);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] LOGOUT: Invalid format\n");
        return;
    }

    if (!validate_uid(uid) || !validate_password(password)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_LOGOUT, STATUS_ERR);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] LOGOUT: Invalid UID or password format (UID=%s)\n", uid);
        return;
    }

    int auth_result = authenticate_user(uid, password);

    if (auth_result == 0) {
        // User não existe → UNR
        snprintf(response, sizeof(response), "%s %s\n", RSP_LOGOUT, STATUS_UNR);
        sendto(sockfd, response, strlen(response), 0, 
            (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] LOGOUT: User %s not registered\n", uid);
        return;
    }

    if (auth_result == -1) {
        // Password errada → WRP
        snprintf(response, sizeof(response), "%s %s\n", RSP_LOGOUT, STATUS_WRP);
        sendto(sockfd, response, strlen(response), 0, 
            (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] LOGOUT: Wrong password for user %s\n", uid);
        return;
    }

    int logged_status = is_user_logged_in(uid);
    if (logged_status == 1) {
        if (logout_user(uid)) {
            snprintf(response, sizeof(response), "%s %s\n", RSP_LOGOUT, STATUS_OK);
            sendto(sockfd, response, strlen(response), 0, 
                (struct sockaddr*)client_addr, addrlen);
            printf("[UDP] LOGOUT: User %s logged out successfully\n", uid);
        } else {
            snprintf(response, sizeof(response), "%s %s\n", RSP_LOGOUT, STATUS_ERR);
            sendto(sockfd, response, strlen(response), 0, 
                (struct sockaddr*)client_addr, addrlen);
            printf("[UDP] LOGOUT: Failed to logout user %s\n", uid);
        }
    } else {
        snprintf(response, sizeof(response), "%s %s\n", RSP_LOGOUT, STATUS_NOK);
        sendto(sockfd, response, strlen(response), 0, 
                (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] LOGOUT: User %s is not logged in\n", uid);
    }
    

}

void handle_my_events(int sockfd, char* message, struct sockaddr_in* client_addr, socklen_t addrlen) {
    char cmd[4], uid[UID_LEN + 1], password[PASSWORD_LEN + 1];
    char response[2048];  // Buffer grande para lista de eventos
    
    // Parse: "LME UID password\n"
    int parsed = sscanf(message, "%3s %6s %8s", cmd, uid, password);
    
    if (parsed != 3) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_MYEVENTS, STATUS_ERR);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MYEVENTS: Invalid format\n");
        return;
    }
    
    // Validar formato
    if (!validate_uid(uid) || !validate_password(password)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_MYEVENTS, STATUS_ERR);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MYEVENTS: Invalid UID or password format (UID=%s)\n", uid);
        return;
    }
    
    // Autenticar utilizador
    int auth_result = authenticate_user(uid, password);
    
    if (auth_result == 0) {
        // Utilizador não existe
        snprintf(response, sizeof(response), "%s %s\n", RSP_MYEVENTS, STATUS_NOK);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MYEVENTS: User %s not registered\n", uid);
        return;
    }
    
    if (auth_result == -1) {
        // Password incorreta
        snprintf(response, sizeof(response), "%s %s\n", RSP_MYEVENTS, STATUS_WRP);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MYEVENTS: Wrong password for user %s\n", uid);
        return;
    }
    
    // Verificar se está logado
    if (!is_user_logged_in(uid)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_MYEVENTS, STATUS_NLG);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MYEVENTS: User %s not logged in\n", uid);
        return;
    }
    
    // Construir caminho para pasta CREATED do utilizador
    char created_path[256];
    snprintf(created_path, sizeof(created_path), "USERS/%s/CREATED", uid);
    
    // Ler lista de eventos criados pelo utilizador
    struct dirent **entries;
    int n = scandir(created_path, &entries, NULL, alphasort);
    
    if (n < 0) {
        // Pasta não existe ou erro ao ler (utilizador sem eventos)
        snprintf(response, sizeof(response), "%s %s\n", RSP_MYEVENTS, STATUS_NOK);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MYEVENTS: User %s has no events\n", uid);
        return;
    }
    
    // Iniciar construção da resposta
    int offset = snprintf(response, sizeof(response), "%s %s", RSP_MYEVENTS, STATUS_OK);
    int event_count = 0;
    
    // Percorrer todos os ficheiros na pasta CREATED
    for (int i = 0; i < n; i++) {
        // Ignorar "." e ".."
        if (strcmp(entries[i]->d_name, ".") == 0 || 
            strcmp(entries[i]->d_name, "..") == 0) {
            free(entries[i]);
            continue;
        }
        
        // Extrair EID do nome do ficheiro (formato: "EID.txt" ou só "EID")
        int eid = atoi(entries[i]->d_name);
        
        if (eid >= 1 && eid <= 999) {
            // Obter estado do evento
            int state = get_event_state(eid);
            
            // Adicionar à resposta: " EID state"
            offset += snprintf(response + offset, sizeof(response) - offset, 
                             " %d %d", eid, state);
            event_count++;
            
            printf("[UDP] MYEVENTS: Found event %03d with state %d for user %s\n", 
                   eid, state, uid);
        }
        
        free(entries[i]);
    }
    free(entries);
    
    // Se não encontrou nenhum evento válido
    if (event_count == 0) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_MYEVENTS, STATUS_NOK);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MYEVENTS: User %s has no valid events\n", uid);
        return;
    }
    
    // Adicionar newline final
    offset += snprintf(response + offset, sizeof(response) - offset, "\n");
    
    // Enviar resposta
    sendto(sockfd, response, strlen(response), 0, 
           (struct sockaddr*)client_addr, addrlen);
    
    printf("[UDP] MYEVENTS: Sent %d event(s) to user %s\n", event_count, uid);
}

void handle_unregister(int sockfd, char* message, struct sockaddr_in* client_addr, socklen_t addrlen) {
    char cmd[4], uid[UID_LEN + 1], password[PASSWORD_LEN + 1];
    char response[32];

    // Parse: "UNR UID password\n"
    int parsed = sscanf(message, "%3s %6s %8s", cmd, uid, password);

    if (parsed != 3) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_UNREGISTER, STATUS_ERR);
        sendto(sockfd, response, strlen(response), 0,
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] UNREGISTER: Invalid format\n");
        return;
    }

    if (!validate_uid(uid) || !validate_password(password)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_UNREGISTER, STATUS_ERR);
        sendto(sockfd, response, strlen(response), 0,
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] UNREGISTER: Invalid UID or password format (UID=%s)\n", uid);
        return;
    }

    int auth_result = authenticate_user(uid, password);
    if (auth_result == 0) {
        // User não existe → UNR
        snprintf(response, sizeof(response), "%s %s\n", RSP_UNREGISTER, STATUS_UNR);
        sendto(sockfd, response, strlen(response), 0,
            (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] UNREGISTER: User %s not registered\n", uid);
        return;
    }

    if (auth_result == -1) {
        // Password errada → WRP
        snprintf(response, sizeof(response), "%s %s\n", RSP_UNREGISTER, STATUS_WRP);
        sendto(sockfd, response, strlen(response), 0,
            (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] UNREGISTER: Wrong password for user %s\n", uid);
        return;
    }

    int logged_status = is_user_logged_in(uid);
    if (logged_status == 1) {
        // User está logged in → OK (fazer logout + unregister)
        logout_user(uid);
        unregister_user(uid);
        snprintf(response, sizeof(response), "%s %s\n", RSP_UNREGISTER, STATUS_OK);
        sendto(sockfd, response, strlen(response), 0,
                (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] UNREGISTER: User %s unregistered successfully\n", uid);
    } else {
        // User NÃO está logged in → NOK
        snprintf(response, sizeof(response), "%s %s\n", RSP_UNREGISTER, STATUS_NOK);
        sendto(sockfd, response, strlen(response), 0,
                (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] UNREGISTER: User %s is not logged in\n", uid);
    }
}

void handle_my_reservations(int sockfd, char* message, struct sockaddr_in* client_addr, socklen_t addrlen) {
    // A implementar

    char cmd[4], uid[UID_LEN + 1], password[PASSWORD_LEN + 1];
    char response[2048];  // Buffer grande para lista de reservas

    // Parse: "LMR UID password\n"
    int parsed = sscanf(message, "%3s %6s %8s", cmd, uid, password);

    if (parsed != 3) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_MY_RESERVATIONS, STATUS_ERR);
        sendto(sockfd, response, strlen(response), 0,
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MY_RESERVATIONS: Invalid format\n");
        return;
    }

    // Validar formato
    if (!validate_uid(uid) || !validate_password(password)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_MY_RESERVATIONS, STATUS_ERR);
        sendto(sockfd, response, strlen(response), 0,
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MY_RESERVATIONS: Invalid UID or password format (UID=%s)\n", uid);
        return;
    }

    // Autenticar utilizador
    int auth_result = authenticate_user(uid, password);

    if (auth_result == 0) {
        // Utilizador não existe
        snprintf(response, sizeof(response), "%s %s\n", RSP_MY_RESERVATIONS, STATUS_NOK);
        sendto(sockfd, response, strlen(response), 0,
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MY_RESERVATIONS: User %s not registered\n", uid);
        return;
    }

    if (auth_result == -1) {
        // Password incorreta
        snprintf(response, sizeof(response), "%s %s\n", RSP_MY_RESERVATIONS, STATUS_WRP);
        sendto(sockfd, response, strlen(response), 0,
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MY_RESERVATIONS: Wrong password for user %s\n", uid);
        return;
    }

    // Verificar se está logado
    if (!is_user_logged_in(uid)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_MY_RESERVATIONS, STATUS_NLG);
        sendto(sockfd, response, strlen(response), 0,
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MY_RESERVATIONS: User %s not logged in\n", uid);
        return;
    }

    char created_path[256];
    snprintf(created_path, sizeof(created_path), "USERS/%s/RESERVED", uid);

    struct dirent **entries;
    int n = scandir(created_path, &entries, NULL, alphasort);

    if (n < 0) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_MY_RESERVATIONS, STATUS_NOK);
        sendto(sockfd, response, strlen(response), 0,
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MY_RESERVATIONS: User %s has no reservations\n", uid);
        return;
    }

    int offset = snprintf(response, sizeof(response), "%s %s", RSP_MY_RESERVATIONS, STATUS_OK);
    int reservation_count = 0;

    for (int i = 0; i < n; i++) {
        if (strcmp(entries[i]->d_name, ".") == 0 ||
            strcmp(entries[i]->d_name, "..") == 0) {
            free(entries[i]);
            continue;
        }

        // Abrir ficheiro de reserva
        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s/%s", created_path, entries[i]->d_name);
        
        FILE *fp = fopen(file_path, "r");
        if (!fp) {
            free(entries[i]);
            continue;
        }
        
        // Ler conteúdo: "EID num_seats dd-mm-yyyy HH:MM:SS"
        int file_eid, num_seats;
        char date[DATE_STR_LEN + 1];
        char time[TIME_STR_LEN + 4];  // Para ler "HH:MM:SS" (8 chars)
        
        if (fscanf(fp, "%d %d %10s %15s", &file_eid, &num_seats, date, time) != 4) {
            fclose(fp);
            free(entries[i]);
            continue;
        }
        fclose(fp);
        // Adicionar à resposta: " EID num_seats date time"
        offset += snprintf(response + offset, sizeof(response) - offset,
                        " %d %s %s %d", file_eid, date, time, num_seats);
        reservation_count++;

        free(entries[i]);
    }
    free(entries);

    if (reservation_count == 0) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_MY_RESERVATIONS, STATUS_NOK);
        sendto(sockfd, response, strlen(response), 0,
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MY_RESERVATIONS: User %s has no valid reservations\n", uid);
        return;
    }

    offset += snprintf(response + offset, sizeof(response) - offset, "\n");

    sendto(sockfd, response, strlen(response), 0,
           (struct sockaddr*)client_addr, addrlen);
        
    printf("[UDP] MY_RESERVATIONS: Sent %d reservation(s) to user %s\n", reservation_count, uid);
    


}
