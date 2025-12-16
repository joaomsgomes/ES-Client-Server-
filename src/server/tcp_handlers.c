#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include "../../include/es_server.h"
#include "../../include/protocol.h"
#include "../../include/user_management.h"
#include "../../include/event_database.h"
#include "../../include/utils.h"
#include "../../include/file_system.h"

void handle_change_password(int client_fd, char* buffer, ssize_t n) {

    (void)n; // Unused parameter

    char cmd[4], uid[UID_LEN + 1], old_password[PASSWORD_LEN + 1], new_password[PASSWORD_LEN + 1];
    char response[64];

    int parsed = sscanf(buffer, "%3s %6s %8s %8s", cmd, uid, old_password, new_password);

    if (parsed != 4) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CHANGE_PASS, STATUS_ERR);
        write(client_fd, response, strlen(response));
        printf("[TCP] CHANGE_PASS: Invalid format (parsed=%d)\n", parsed);
        return;
    }

    if (!validate_uid(uid) || !validate_password(old_password) || !validate_password(new_password)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CHANGE_PASS, STATUS_ERR);
        write(client_fd, response, strlen(response));
        printf("[TCP] CHANGE_PASS: Invalid UID or password format (UID=%s)\n", uid);
        return;
    }

    int auth_result = authenticate_user(uid, old_password);

    if (auth_result == 0) {
        // User não existe
        snprintf(response, sizeof(response), "%s %s\n", RSP_CHANGE_PASS, STATUS_NID);
        write(client_fd, response, strlen(response));
        printf("[TCP] CHANGE_PASS: User not found (UID=%s)\n", uid);
        return;
    }

    if (auth_result == -1) {
        // Password incorreta
        snprintf(response, sizeof(response), "%s %s\n", RSP_CHANGE_PASS, STATUS_NOK);
        write(client_fd, response, strlen(response));
        printf("[TCP] CHANGE_PASS: Wrong old password (UID=%s)\n", uid);
        return;
    }

    if (!is_logged_in(uid)) {
        // User não está logged in
        snprintf(response, sizeof(response), "%s %s\n", RSP_CHANGE_PASS, STATUS_NLG);
        write(client_fd, response, strlen(response));
        printf("[TCP] CHANGE_PASS: User not logged in (UID=%s)\n", uid);
        return;
    }

    if (change_password(uid, old_password, new_password)) {
        // Sucesso
        snprintf(response, sizeof(response), "%s %s\n", RSP_CHANGE_PASS, STATUS_OK);
        write(client_fd, response, strlen(response));
        printf("[TCP] CHANGE_PASS: Password changed successfully for user %s\n", uid);
    } else {
        // Falha ao alterar password
        snprintf(response, sizeof(response), "%s %s\n", RSP_CHANGE_PASS, STATUS_NOK);
        write(client_fd, response, strlen(response));
        printf("[TCP] CHANGE_PASS: Failed to change password for user %s\n", uid);
    }

}

void handle_create_event(int client_fd, char* buffer, ssize_t n) {
    char cmd[4], uid[UID_LEN + 1], password[PASSWORD_LEN + 1];
    char name[EVENT_NAME_LEN + 1], date[DATE_STR_LEN + 1], time[TIME_STR_LEN + 1];
    char filename[FILENAME_LEN + 1];
    int attendance;
    long filesize;
    char response[64];
    
    printf("[TCP] CREATE: Received %zd bytes total\n", n);
    printf("[TCP] CREATE: Buffer content (first 100 bytes): '%.*s'\n", (int)(n > 100 ? 100 : n), buffer);
    
    // Parse do comando (sem Fdata ainda):
    // CRE UID password name date time attendance Fname Fsize
    int parsed = sscanf(buffer, "%3s %6s %8s %10s %10s %5s %d %24s %ld",
                       cmd, uid, password, name, date, time, &attendance, filename, &filesize);
    
    printf("[TCP] CREATE: Parsed %d fields: cmd=%s uid=%s name=%s date=%s time=%s attendance=%d fname=%s fsize=%ld\n",
           parsed, cmd, uid, name, date, time, attendance, filename, filesize);
    
    if (parsed != 9) {
        // Formato inválido
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_ERR);
        write(client_fd, response, strlen(response));
        printf("[TCP] CREATE: Invalid format (parsed=%d)\n", parsed);
        return;
    }
    
    // Validar parâmetros
    if (!validate_uid(uid) || !validate_password(password)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_ERR);
        write(client_fd, response, strlen(response));
        printf("[TCP] CREATE: Invalid UID or password format\n");
        return;
    }
    
    if (!validate_event_name(name)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_ERR);
        write(client_fd, response, strlen(response));
        printf("[TCP] CREATE: Invalid event name\n");
        return;
    }
    
    if (!validate_datetime_format(date, time)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_ERR);
        write(client_fd, response, strlen(response));
        printf("[TCP] CREATE: Invalid format - use dd-mm-yyyy hh:mm\n");
        return;
    }
    
    if (!validate_datetime_range(date, time)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_ERR);
        write(client_fd, response, strlen(response));
        printf("[TCP] CREATE: Date/time out of valid range\n");
        return;
    }
    
    // Validar que a data/hora não é passada
    char full_datetime[20];
    snprintf(full_datetime, sizeof(full_datetime), "%s %s", date, time);
    if (is_date_before_now(full_datetime)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_ERR);
        write(client_fd, response, strlen(response));
        printf("[TCP] CREATE: Event date cannot be in the past\n");
        return;
    }
    
    if (attendance < 10 || attendance > 999) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_ERR);
        write(client_fd, response, strlen(response));
        printf("[TCP] CREATE: Invalid attendance (%d)\n", attendance);
        return;
    }
    
    if (filesize <= 0 || filesize > MAX_FILE_SIZE) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_ERR);
        write(client_fd, response, strlen(response));
        printf("[TCP] CREATE: Invalid file size (%ld)\n", filesize);
        return;
    }
    
    // Autenticar utilizador
    int auth_result = authenticate_user(uid, password);
    
    if (auth_result == 0) {
        // Utilizador não existe
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_NLG);
        write(client_fd, response, strlen(response));
        printf("[TCP] CREATE: User not found (UID=%s)\n", uid);
        return;
    }
    
    if (auth_result == -1) {
        // Password incorreta
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_WRP);
        write(client_fd, response, strlen(response));
        printf("[TCP] CREATE: Wrong password (UID=%s)\n", uid);
        return;
    }
    
    // Encontrar posição do Fdata no buffer
    // O formato é: "CRE UID password name date time attendance Fname Fsize "
    // Depois disso vem o Fdata (dados binários)
    
    char *filedata_ptr = buffer;
    int header_fields = 0;
    
    printf("[TCP] CREATE: Looking for file data start position...\n");
    
    // Avançar pelos primeiros 9 campos (CRE, UID, password, name, date, time, attendance, Fname, Fsize)
    while (header_fields < 9 && filedata_ptr < buffer + n) {
        if (*filedata_ptr == ' ') {
            header_fields++;
            printf("[TCP] CREATE: Found field %d at position %ld\n", header_fields, filedata_ptr - buffer);
        }
        filedata_ptr++;
    }
    
    long header_size = filedata_ptr - buffer;
    long remaining_bytes = n - header_size;
    
    printf("[TCP] CREATE: Header size=%ld bytes, remaining=%ld bytes, expected file=%ld bytes\n", 
           header_size, remaining_bytes, filesize);
    
    // Verificar se temos dados suficientes
    if (remaining_bytes < filesize) {
        // Faltam dados - precisamos ler mais do socket
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_ERR);
        write(client_fd, response, strlen(response));
        printf("[TCP] CREATE: Incomplete file data (expected=%ld, got=%ld)\n", 
               filesize, remaining_bytes);
        return;
    }
    
    // Alocar memória para filedata
    unsigned char *filedata = malloc(filesize);
    if (!filedata) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CREATE, STATUS_NOK);
        write(client_fd, response, strlen(response));
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
    
    strncpy(ev.time, time, TIME_STR_LEN);
    ev.time[TIME_STR_LEN] = '\0';
    
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
        write(client_fd, response, strlen(response));
        free(filedata);
        printf("[TCP] CREATE: Failed to create event\n");
        return;
    }
    
    // Sucesso! Retornar EID gerado
    snprintf(response, sizeof(response), "%s %s %03d\n", RSP_CREATE, STATUS_OK, ev.eid);
    write(client_fd, response, strlen(response));
    
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
        write(client_fd, response, strlen(response));
        printf("[TCP] CLOSE: Invalid format\n");
        return;
    }
    
    if (!validate_uid(uid) || !validate_password(password)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CLOSE, STATUS_ERR);
        write(client_fd, response, strlen(response));
        return;
    }
    
    if (eid < 1 || eid > 999) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CLOSE, STATUS_ERR);
        write(client_fd, response, strlen(response));
        return;
    }
    
    int auth_result = authenticate_user(uid, password);
    
    if (auth_result == 0) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CLOSE, STATUS_NOK);
        write(client_fd, response, strlen(response));
        return;
    }
    
    if (!is_user_logged_in(uid)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_CLOSE, STATUS_NLG);
        write(client_fd, response, strlen(response));
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
    write(client_fd, response, strlen(response));
    
    printf("[TCP] CLOSE: EID=%03d, UID=%s, status=%s\n", eid, uid, status);
}

// DELETE DEBUG PRINTS AFTER IMPLEMENTING RESERVE !!

void handle_list_events(int client_fd, char* buffer, ssize_t bytes_read) {
    (void)bytes_read;
    
    printf("[TCP] LIST: Received buffer: '%s' (%zd bytes)\n", buffer, bytes_read);
    
    char cmd[4];
    char response[8192];  // Buffer grande para múltiplos eventos
    
    // Parse: "LST\n"
    int parsed = sscanf(buffer, "%3s", cmd);
    printf("[TCP] LIST: Parsed command: '%s' (parsed=%d)\n", cmd, parsed);
    
    if (parsed != 1) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_LIST, STATUS_ERR);
        write(client_fd, response, strlen(response));
        printf("[TCP] LIST: Invalid format\n");
        return;
    }
    
    // Ler pasta EVENTS/ para obter todos os eventos
    struct dirent **entries;
    int n = scandir("EVENTS", &entries, NULL, alphasort);
    
    if (n < 0) {
        // Pasta não existe ou erro ao ler
        snprintf(response, sizeof(response), "%s %s\n", RSP_LIST, STATUS_NOK);
        write(client_fd, response, strlen(response));
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
            
            // Auto-fechar se o evento já passou
            auto_close_if_past(eid);
            
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
        write(client_fd, response, strlen(response));
        printf("[TCP] LIST: No valid events found\n");
        return;
    }
    
    // Adicionar newline final
    offset += snprintf(response + offset, sizeof(response) - offset, "\n");
    
    // Enviar resposta
    write(client_fd, response, strlen(response));
    
    printf("[TCP] LIST: Sent %d event(s)\n", event_count);
}

void handle_reserve_seats(int client_fd, char* buffer, ssize_t bytes_read) {


    (void)bytes_read;
    
    char cmd[4], uid[UID_LEN + 1], password[PASSWORD_LEN + 1], eid_str[4];
    int people, eid;
    char response[64];

    
    int parsed = sscanf(buffer, "%3s %6s %8s %3s %d", 
                       cmd, uid, password, eid_str, &people);
    eid = atoi(eid_str);

    if (parsed != 5) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_RESERVE, STATUS_ERR);
        write(client_fd, response, strlen(response));
        printf("[TCP] RESERVE: Invalid format (parsed=%d)\n", parsed);
        return;
    }

    if (!validate_uid(uid) || !validate_password(password)) {
        printf("Validate_uid and validate_password failed\n");
        snprintf(response, sizeof(response), "%s %s\n", RSP_RESERVE, STATUS_ERR);
        write(client_fd, response, strlen(response));
        return;
    }

    if (eid < 1 || eid > 999 || people < 1 || people > 999) {
        printf("[TCP] RESERVE: Invalid EID (%d) or number of people (%d)\n", eid, people);
        snprintf(response, sizeof(response), "%s %s\n", RSP_RESERVE, STATUS_ERR);
        write(client_fd, response, strlen(response));
        return;
    }

    
    if (!user_exists(uid)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_RESERVE, STATUS_NLG);
        write(client_fd, response, strlen(response));
        printf("[TCP] RESERVE: User not found (UID=%s)\n", uid);
        return;
    }

   
    int auth_result = authenticate_user(uid, password);
    if (auth_result != 1) {
        snprintf(response, sizeof(response), "%s %s\n", 
                RSP_RESERVE, auth_result == 0 ? STATUS_NLG : STATUS_WRP);
        write(client_fd, response, strlen(response));
        printf("[TCP] RESERVE: Authentication failed (UID=%s)\n", uid);
        return;
    }

    
    if (!is_logged_in(uid)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_RESERVE, STATUS_NLG);
        write(client_fd, response, strlen(response));
        printf("[TCP] RESERVE: User not logged in (UID=%s)\n", uid);
        return;
    }

    
    if (!event_exists(eid)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_RESERVE, STATUS_NOK);
        write(client_fd, response, strlen(response));
        printf("[TCP] RESERVE: Event does not exist (EID=%03d)\n", eid);
        return;
    }

    // Auto-fechar se o evento já passou
    auto_close_if_past(eid);

    int event_state = get_event_state(eid);
    
    if (event_state == 0) {  // Past
        snprintf(response, sizeof(response), "%s %s\n", RSP_RESERVE, STATUS_PST);
        write(client_fd, response, strlen(response));
        printf("[TCP] RESERVE: Event date has passed (EID=%03d)\n", eid);
        return;
    }

    if (event_state == 3) {  // Closed
        snprintf(response, sizeof(response), "%s %s\n", RSP_RESERVE, STATUS_CLS);
        write(client_fd, response, strlen(response));
        printf("[TCP] RESERVE: Event closed (EID=%03d)\n", eid);
        return;
    }

    if (event_state == 2) {  // Sold out
        snprintf(response, sizeof(response), "%s %s\n", RSP_RESERVE, STATUS_SLD);
        write(client_fd, response, strlen(response));
        printf("[TCP] RESERVE: Event sold out (EID=%03d)\n", eid);
        return;
    }

    if (event_state != 1) {  // Not active (should not happen) REVIEW !
        snprintf(response, sizeof(response), "%s %s\n", RSP_RESERVE, STATUS_NOK);
        write(client_fd, response, strlen(response));
        printf("[TCP] RESERVE: Event not active (EID=%03d, state=%d)\n", eid, event_state);
        return;
    }

    // EVENTO ESTÁ ACTIVE (state=1)
    
    // Obter total de lugares e lugares já reservados
    int total_seats, reserved_seats;
    if (!get_event_seats(eid, &total_seats, &reserved_seats)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_RESERVE, STATUS_ERR);
        write(client_fd, response, strlen(response));
        printf("[TCP] RESERVE: Failed to read event seats (EID=%03d)\n", eid);
        return;
    }

    int available_seats = total_seats - reserved_seats;

    if (people > available_seats) {
        // REJEITAR - enviar número de lugares disponíveis
        snprintf(response, sizeof(response), "%s %s %d\n", 
                RSP_RESERVE, STATUS_REJ, available_seats);
        write(client_fd, response, strlen(response));
        printf("[TCP] RESERVE: Not enough seats (EID=%03d, requested=%d, available=%d)\n", 
               eid, people, available_seats);
        return;
    }

    // ACEITAR RESERVA - criar ficheiros de reserva
    if (!create_reservation(eid, uid, people)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_RESERVE, STATUS_ERR);
        write(client_fd, response, strlen(response));
        printf("[TCP] RESERVE: Failed to create reservation (EID=%03d, UID=%s)\n", eid, uid);
        return;
    }

    snprintf(response, sizeof(response), "%s %s\n", RSP_RESERVE, STATUS_ACC);
    write(client_fd, response, strlen(response));
    printf("[TCP] RESERVE: Success (EID=%03d, UID=%s, seats=%d)\n", eid, uid, people);
}





void handle_show_event(int client_fd, char* buffer, ssize_t bytes_read) {
    (void)bytes_read;

    char cmd[4];
    int eid;
    char response[128];
    
    // Parse: "SED EID\n"
    int parsed = sscanf(buffer, "%3s %d", cmd, &eid);

    if (parsed != 2) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_SHOW, STATUS_NOK);
        write(client_fd, response, strlen(response));
        printf("[TCP] SHOW: Invalid format\n");
        return;
    }

    if (eid < 1 || eid > 999) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_SHOW, STATUS_ERR);
        write(client_fd, response, strlen(response));
        printf("[TCP] SHOW: Invalid EID\n");
        return;
    }

    // Ler informações do evento do ficheiro START_eid.txt
    // Auto-fechar se o evento já passou
    auto_close_if_past(eid);
    
    Event ev;
    
    if (get_event(eid, &ev) != 0) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_SHOW, STATUS_NOK);
        write(client_fd, response, strlen(response));
        return;
    }

    // Verificar se filedata é válido
    if (ev.filedata == NULL) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_SHOW, STATUS_NOK);
        write(client_fd, response, strlen(response));
        printf("[TCP] SHOW: Event %03d not found\n", eid);
        return;
    }

    // Construir header (só texto)
    // RSE OK UID name date time attendance reserved Fname Fsize
    char header[512];
    int header_len = snprintf(header, sizeof(header),
                              "%s %s %s %s %s %s %d %d %s %ld ",
                              RSP_SHOW, STATUS_OK,
                              ev.uid, ev.name, ev.date, ev.time,
                              ev.total_seats, ev.reserved_seats,
                              ev.filename, ev.file_size);

    if (header_len < 0 || header_len >= (int)sizeof(header)) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_SHOW, STATUS_NOK);
        write(client_fd, response, strlen(response));
        printf("[TCP] SHOW: Header construction failed for EID=%03d\n", eid);
        return;
    }

    size_t total_size = header_len + ev.file_size + 1;
    
    char *full_message = malloc(total_size);
    if (!full_message) {
        snprintf(response, sizeof(response), "%s %s\n", RSP_SHOW, STATUS_NOK);
        write(client_fd, response, strlen(response));
        printf("[TCP] SHOW: Memory allocation failed for event %03d\n", eid);
        return;
    }

    memcpy(full_message, header, header_len);
    memcpy(full_message + header_len, ev.filedata, ev.file_size);
    full_message[header_len + ev.file_size] = '\n';

    size_t total_sent = 0;
    while (total_sent < total_size) {
        ssize_t n = write(client_fd, full_message + total_sent, total_size - total_sent);
        if (n <= 0) {
            perror("[TCP] SHOW: Error sending data");
            free(full_message);
            return;
        }
        total_sent += n;
    }

    free(full_message);
    printf("[TCP] SHOW: Event %03d sent successfully (%ld bytes)\n", eid, ev.file_size);

}




















