#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include "../../include/es_server.h"
#include "../../include/protocol.h"
#include "../../include/user_management.h"
#include "../../include/utils.h"
#include "../../include/file_system.h"



void handle_create_event(int client_fd, char* buffer, ssize_t n) {
    char cmd[4], uid[UID_LEN + 1], password[PASSWORD_LEN + 1];
    char name[EVENT_NAME_LEN + 1], date[DATE_STR_LEN + 1];
    char filename[FILENAME_LEN + 1];
    int attendance;
    long filesize;
    char response[64];
    
    // Parse do comando (sem Fdata ainda):
    // CRE UID password name date attendance Fname Fsize
    int parsed = sscanf(buffer, "%3s %6s %8s %10s %10s %d %24s %ld",
                       cmd, uid, password, name, date, &attendance, filename, &filesize);
    
    if (parsed != 8) {
        // Formato inválido
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_ERR);
        send(client_fd, response, strlen(response), 0);
        printf("[TCP] CREATE: Invalid format (parsed=%d)\n", parsed);
        return;
    }
    
    // Validar parâmetros
    if (!validate_uid(uid) || !validate_password(password)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_ERR);
        send(client_fd, response, strlen(response), 0);
        printf("[TCP] CREATE: Invalid UID or password format\n");
        return;
    }
    
    if (!validate_event_name(name)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_ERR);
        send(client_fd, response, strlen(response), 0);
        printf("[TCP] CREATE: Invalid event name\n");
        return;
    }
    
    if (!validate_date(date)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_ERR);
        send(client_fd, response, strlen(response), 0);
        printf("[TCP] CREATE: Invalid date format\n");
        return;
    }
    
    if (attendance < 10 || attendance > 999) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_ERR);
        send(client_fd, response, strlen(response), 0);
        printf("[TCP] CREATE: Invalid attendance (%d)\n", attendance);
        return;
    }
    
    if (filesize <= 0 || filesize > MAX_FILE_SIZE) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_ERR);
        send(client_fd, response, strlen(response), 0);
        printf("[TCP] CREATE: Invalid file size (%ld)\n", filesize);
        return;
    }
    
    // Autenticar utilizador
    int auth_result = authenticate_user(uid, password);
    
    if (auth_result == 0) {
        // Utilizador não existe
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_NLG);
        send(client_fd, response, strlen(response), 0);
        printf("[TCP] CREATE: User not found (UID=%s)\n", uid);
        return;
    }
    
    if (auth_result == -1) {
        // Password incorreta
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_WRP);
        send(client_fd, response, strlen(response), 0);
        printf("[TCP] CREATE: Wrong password (UID=%s)\n", uid);
        return;
    }
    
    // Encontrar posição do Fdata no buffer
    // O formato é: "CRE UID password name date attendance Fname Fsize "
    // Depois disso vem o Fdata (dados binários)
    
    char *filedata_ptr = buffer;
    int header_fields = 0;
    
    // Avançar pelos primeiros 8 campos (CRE, UID, password, name, date, attendance, Fname, Fsize)
    while (header_fields < 8 && filedata_ptr < buffer + n) {
        if (*filedata_ptr == ' ') {
            header_fields++;
        }
        filedata_ptr++;
    }
    
    // Verificar se temos dados suficientes
    long remaining_bytes = n - (filedata_ptr - buffer);
    
    if (remaining_bytes < filesize) {
        // Faltam dados - precisamos ler mais do socket
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_ERR);
        send(client_fd, response, strlen(response), 0);
        printf("[TCP] CREATE: Incomplete file data (expected=%ld, got=%ld)\n", 
               filesize, remaining_bytes);
        return;
    }
    
    // Alocar memória para filedata
    unsigned char *filedata = malloc(filesize);
    if (!filedata) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_NOK);
        send(client_fd, response, strlen(response), 0);
        printf("[TCP] CREATE: Memory allocation failed\n");
        return;
    }
    
    // Copiar filedata
    memcpy(filedata, filedata_ptr, filesize);
    
    // Criar estrutura Event
    Event ev;
    memset(&ev, 0, sizeof(Event));
    
    strncpy(ev.uid, uid, UID_LEN);
    ev.uid[UID_LEN] = '\0';
    
    strncpy(ev.name, name, EVENT_NAME_LEN);
    ev.name[EVENT_NAME_LEN] = '\0';
    
    strncpy(ev.date, date, DATE_STR_LEN);
    ev.date[DATE_STR_LEN] = '\0';
    
    ev.total_seats = attendance;
    ev.reserved_seats = 0;
    
    strncpy(ev.filename, filename, FILENAME_LEN);
    ev.filename[FILENAME_LEN] = '\0';
    
    ev.file_size = filesize;
    ev.filedata = filedata;
    ev.state = 1; // EVENT_ACTIVE
    
    // Criar evento no sistema de ficheiros
    int rc = create_event(&ev);
    
    if (rc != 0) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_NOK);
        send(client_fd, response, strlen(response), 0);
        free(filedata);
        printf("[TCP] CREATE: Failed to create event\n");
        return;
    }
    
    // Sucesso! Retornar EID gerado
    snprintf(response, sizeof(response), "%s %s %03d\n", RSP_CREATE, STATUS_OK, ev.eid);
    send(client_fd, response, strlen(response), 0);
    
    free(filedata);
    
    printf("[TCP] CREATE: Event created successfully (EID=%03d, UID=%s, name=%s)\n",
           ev.eid, uid, name);
}


/**
 * Handler para comando CLOSE
 * Protocolo: CLS UID password EID\n
 * Resposta: RCL status\n
 */
void handle_close_event(int client_fd, char* buffer, ssize_t n) {

    (void)n;
    
    char cmd[4], uid[UID_LEN + 1], password[PASSWORD_LEN + 1];
    int eid;
    char response[64];
    
    int parsed = sscanf(buffer, "%3s %6s %8s %d", cmd, uid, password, &eid);
    
    if (parsed != 4) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CLOSE, STATUS_ERR);
        send(client_fd, response, strlen(response), 0);
        printf("[TCP] CLOSE: Invalid format\n");
        return;
    }
    
    if (!validate_uid(uid) || !validate_password(password)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CLOSE, STATUS_ERR);
        send(client_fd, response, strlen(response), 0);
        return;
    }
    
    if (eid < 1 || eid > 999) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CLOSE, STATUS_ERR);
        send(client_fd, response, strlen(response), 0);
        return;
    }
    
    int auth_result = authenticate_user(uid, password);
    
    if (auth_result == 0) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CLOSE, STATUS_NOK);
        send(client_fd, response, strlen(response), 0);
        return;
    }
    
    if (auth_result == -1) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CLOSE, STATUS_WRP);
        send(client_fd, response, strlen(response), 0);
        return;
    }
    
    if (!is_user_logged_in(uid)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CLOSE, STATUS_NLG);
        send(client_fd, response, strlen(response), 0);
        return;
    }
    
    int close_result = close_event(uid, eid);
    
    const char *status;
    switch (close_result) {
        case 0:  status = STATUS_OK; break;
        case -1: status = STATUS_NOE; break;
        case -2: status = STATUS_CLO; break;
        case -3: status = STATUS_EOW; break;
        case 1:  status = STATUS_PST; break;
        case 2:  status = STATUS_SLD; break;
        default: status = STATUS_ERR; break;
    }
    
    snprintf(response, sizeof(response), "%s %s\n", RSP_CLOSE, status);
    send(client_fd, response, strlen(response), 0);
    
    printf("[TCP] CLOSE: EID=%03d, UID=%s, status=%s\n", eid, uid, status);
}

void handle_list_events(int client_fd, char* buffer, ssize_t bytes_read) {
    (void)bytes_read;
    
    char cmd[4];
    char response[8192];  // Buffer grande para múltiplos eventos
    
    // Parse: "LST\n"
    int parsed = sscanf(buffer, "%3s", cmd);
    
    if (parsed != 1) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_LIST, STATUS_ERR);
        send(client_fd, response, strlen(response), 0);
        printf("[TCP] LIST: Invalid format\n");
        return;
    }
    
    // Ler pasta EVENTS/ para obter todos os eventos
    struct dirent **entries;
    int n = scandir("EVENTS", &entries, NULL, alphasort);
    
    if (n < 0) {
        // Pasta não existe ou erro ao ler
        snprintf(response, sizeof(response), "%s %s\n", RSP_LIST, STATUS_NOK);
        send(client_fd, response, strlen(response), 0);
        printf("[TCP] LIST: No events directory found\n");
        return;
    }
    
    // Iniciar construção da resposta
    int offset = snprintf(response, sizeof(response), "%s %s", RSP_LIST, STATUS_OK);
    int event_count = 0;
    
    // Percorrer todos os eventos na pasta EVENTS
    for (int i = 0; i < n; i++) {
        // Ignorar "." e ".."
        if (strcmp(entries[i]->d_name, ".") == 0 || 
            strcmp(entries[i]->d_name, "..") == 0) {
            free(entries[i]);
            continue;
        }
        
        // Extrair EID do nome da pasta (formato: "001", "002", etc)
        int eid = atoi(entries[i]->d_name);
        
        if (eid >= 1 && eid <= 999) {
            // Ler informações do evento do ficheiro START_eid.txt
            char start_file[128];
            snprintf(start_file, sizeof(start_file), "EVENTS/%03d/START_%03d.txt", eid, eid);
            
            FILE *fp = fopen(start_file, "r");
            if (!fp) {
                free(entries[i]);
                continue;  // Evento sem ficheiro START, ignorar
            }
            
            // Ler conteúdo: "UID name desc_fname total_seats start_date start_time"
            char event_uid[UID_LEN + 1];
            char event_name[EVENT_NAME_LEN + 1];
            char desc_fname[FILENAME_LEN + 1];
            int total_seats;
            char event_date[DATE_STR_LEN + 1];
            char event_time[6];  // hh:mm
            
            int fields = fscanf(fp, "%6s %10s %24s %d %10s %5s",
                               event_uid, event_name, desc_fname, 
                               &total_seats, event_date, event_time);
            fclose(fp);
            
            if (fields != 6) {
                free(entries[i]);
                continue;  // Formato inválido, ignorar
            }
            
            // Obter estado do evento
            int state = get_event_state(eid);
            
            // Adicionar à resposta: " EID name state event_date"
            // Formato completo da data: "dd-mm-yyyy hh:mm"
            offset += snprintf(response + offset, sizeof(response) - offset, 
                             " %d %s %d %s %s", 
                             eid, event_name, state, event_date, event_time);
            event_count++;
            
            printf("[TCP] LIST: Found event %03d: name=%s, state=%d, date=%s %s\n", 
                   eid, event_name, state, event_date, event_time);
        }
        
        free(entries[i]);
    }
    free(entries);
    
    // Se não encontrou nenhum evento válido
    if (event_count == 0) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_LIST, STATUS_NOK);
        send(client_fd, response, strlen(response), 0);
        printf("[TCP] LIST: No valid events found\n");
        return;
    }
    
    // Adicionar newline final
    offset += snprintf(response + offset, sizeof(response) - offset, "\n");
    
    // Enviar resposta
    send(client_fd, response, strlen(response), 0);
    
    printf("[TCP] LIST: Sent %d event(s)\n", event_count);
}




















