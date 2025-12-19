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
#include "../../include/event_database.h"
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
    ssize_t sent;
    
    // Parse: "LIN UID password\n"
    int parsed = sscanf(message, "%3s %6s %8s", cmd, uid, password);
    
    if (parsed != 3) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_LOGIN, STATUS_ERR);
        sent = sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        if (sent < 0) {
            perror("[UDP] LOGIN: sendto failed");
        }
        printf("[UDP] LOGIN: Invalid format\n");
        return;
    }
    
    if (!validate_uid(uid) || !validate_password(password)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_LOGIN, STATUS_ERR);
        sent = sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        if (sent < 0) {
            perror("[UDP] LOGIN: sendto failed");
        }
        printf("[UDP] LOGIN: Invalid UID or password format (UID=%s)\n", uid);
        return;
    }
    
    int auth_result = authenticate_user(uid, password);
    
    if (auth_result == 1) {
        login_user(uid);
        snprintf(response, sizeof(response), "%s %s\n", RSP_LOGIN, STATUS_OK);
        sent = sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        if (sent < 0) {
            perror("[UDP] LOGIN: sendto failed");
        }
        printf("[UDP] LOGIN: User %s logged in successfully\n", uid);
        
    } else if (auth_result == 0) {
        if (register_user(uid, password)) {
            login_user(uid);
            snprintf(response, sizeof(response), "%s %s\n", RSP_LOGIN, STATUS_REG);
            sent = sendto(sockfd, response, strlen(response), 0, 
                   (struct sockaddr*)client_addr, addrlen);
            if (sent < 0) {
                perror("[UDP] LOGIN: sendto failed");
            }
            printf("[UDP] LOGIN: New user %s registered successfully\n", uid);
        } else {
            snprintf(response, sizeof(response), "%s %s\n", RSP_LOGIN, STATUS_ERR);
            sent = sendto(sockfd, response, strlen(response), 0, 
                   (struct sockaddr*)client_addr, addrlen);
            if (sent < 0) {
                perror("[UDP] LOGIN: sendto failed");
            }
            printf("[UDP] LOGIN: Failed to register user %s\n", uid);
        }
        
    } else {
        // auth_result == -1: Password incorreta
        snprintf(response, sizeof(response), "%s %s\n", RSP_LOGIN, STATUS_NOK);
        sent = sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        if (sent < 0) {
            perror("[UDP] LOGIN: sendto failed");
        }
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
    char response[4096]; 

    // Parse: "LME UID password\n"
    int parsed = sscanf(message, "%3s %6s %8s", cmd, uid, password);
    
    if (parsed != 3) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_MYEVENTS, STATUS_ERR);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MYEVENTS: Invalid format\n");
        return;
    }
    
    if (!validate_uid(uid) || !validate_password(password)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_MYEVENTS, STATUS_ERR);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MYEVENTS: Invalid UID or password format (UID=%s)\n", uid);
        return;
    }
    

    int auth_result = authenticate_user(uid, password);
    
    if (auth_result == 0) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_MYEVENTS, STATUS_NOK);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MYEVENTS: User %s not registered\n", uid);
        return;
    }
    
    if (auth_result == -1) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_MYEVENTS, STATUS_WRP);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MYEVENTS: Wrong password for user %s\n", uid);
        return;
    }
    
    if (!is_user_logged_in(uid)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_MYEVENTS, STATUS_NLG);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MYEVENTS: User %s not logged in\n", uid);
        return;
    }
    
    char created_path[256];
    snprintf(created_path, sizeof(created_path), "USERS/%s/CREATED", uid);
    
    struct dirent **entries;
    int n = scandir(created_path, &entries, NULL, alphasort);
    
    if (n < 0) {
        
        snprintf(response, sizeof(response), "%s %s\n", RSP_MYEVENTS, STATUS_NOK);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MYEVENTS: User %s has no events\n", uid);
        return;
    }
    
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
        
        
        int eid = atoi(entries[i]->d_name);
        
        if (eid >= 1 && eid <= 999) {
            // Auto-fechar se o evento já passou
            auto_close_if_past(eid);
            
            
            int state = get_event_state(eid);
            
            
            offset += snprintf(response + offset, sizeof(response) - offset, 
                             " %03d %d", eid, state);
            event_count++;
            
            printf("[UDP] MYEVENTS: Found event %03d with state %d for user %s\n", 
                   eid, state, uid);
        }
        
        free(entries[i]);
    }
    free(entries);
    
    if (event_count == 0) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_MYEVENTS, STATUS_NOK);
        sendto(sockfd, response, strlen(response), 0, 
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MYEVENTS: User %s has no valid events\n", uid);
        return;
    }
    
    offset += snprintf(response + offset, sizeof(response) - offset, "\n");
    
    
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

    char cmd[4], uid[UID_LEN + 1], password[PASSWORD_LEN + 1];
    char response[4096];  // Buffer grande para lista de reservas até 50

    // Parse: "LMR UID password\n"
    int parsed = sscanf(message, "%3s %6s %8s", cmd, uid, password);

    if (parsed != 3) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_MY_RESERVATIONS, STATUS_ERR);
        sendto(sockfd, response, strlen(response), 0,
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MY_RESERVATIONS: Invalid format\n");
        return;
    }

    if (!validate_uid(uid) || !validate_password(password)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_MY_RESERVATIONS, STATUS_ERR);
        sendto(sockfd, response, strlen(response), 0,
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MY_RESERVATIONS: Invalid UID or password format (UID=%s)\n", uid);
        return;
    }

    
    int auth_result = authenticate_user(uid, password);

    if (auth_result == 0) {
        
        snprintf(response, sizeof(response), "%s %s\n", RSP_MY_RESERVATIONS, STATUS_NOK);
        sendto(sockfd, response, strlen(response), 0,
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MY_RESERVATIONS: User %s not registered\n", uid);
        return;
    }

    if (auth_result == -1) {
        
        snprintf(response, sizeof(response), "%s %s\n", RSP_MY_RESERVATIONS, STATUS_WRP);
        sendto(sockfd, response, strlen(response), 0,
               (struct sockaddr*)client_addr, addrlen);
        printf("[UDP] MY_RESERVATIONS: Wrong password for user %s\n", uid);
        return;
    }

    
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

    // Ler todas as reservas para array
    Reservation reservations[50];
    int reservation_count = 0;

    for (int i = 0; i < n && reservation_count < 50; i++) {
        if (strcmp(entries[i]->d_name, ".") == 0 || strcmp(entries[i]->d_name, "..") == 0) {
            free(entries[i]);
            continue;
        }

        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s/%s", created_path, entries[i]->d_name);
        
        FILE *fp = fopen(file_path, "r");
        if (!fp) {
            free(entries[i]);
            continue;
        }
        
        int file_eid, num_seats;
        char date[DATE_STR_LEN + 1];
        char time[TIME_STR_LEN + 4];
        
        if (fscanf(fp, "%d %d %10s %15s", &file_eid, &num_seats, date, time) == 4) {
            reservations[reservation_count].eid = file_eid;
            reservations[reservation_count].num_seats = num_seats;
            strncpy(reservations[reservation_count].date, date, sizeof(reservations[reservation_count].date) - 1);
            strncpy(reservations[reservation_count].time, time, sizeof(reservations[reservation_count].time) - 1);
            snprintf(reservations[reservation_count].datetime, sizeof(reservations[reservation_count].datetime), 
                     "%s %s", date, time);
            reservation_count++;
        }
        fclose(fp);
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

    // Ordenar por data (mais recente primeiro)
    qsort(reservations, reservation_count, sizeof(Reservation), compare_reservations_desc);

    // Construir resposta com reservas ordenadas
    int offset = snprintf(response, sizeof(response), "%s %s", RSP_MY_RESERVATIONS, STATUS_OK);
    for (int i = 0; i < reservation_count; i++) {
        offset += snprintf(response + offset, sizeof(response) - offset,
                          " %03d %s %s %d", 
                          reservations[i].eid, 
                          reservations[i].date, 
                          reservations[i].time, 
                          reservations[i].num_seats);
    }
    offset += snprintf(response + offset, sizeof(response) - offset, "\n");

    sendto(sockfd, response, strlen(response), 0,
           (struct sockaddr*)client_addr, addrlen);
        
    printf("[UDP] MY_RESERVATIONS: Sent %d reservation(s) to user %s\n", reservation_count, uid);
    


}


