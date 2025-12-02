#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../../include/es_server.h"
#include "../../include/file_system.h"
#include "../../include/utils.h"

/**
 * Inicializa o sistema de eventos
 */
void init_event_system() {
    printf("[INIT] Event system ready (file-based)\n");
}

/**
 * Cria um novo evento
 * Formato START file: UID name desc_fname total_seats start_date start_time
 * Exemplo: 123456 Concert poster.jpg 500 31-12-2025 20:00
 */
int create_event(Event *ev) {
    if (!ev) return -1;
    
    // Obter próximo EID disponível
    int eid = get_next_eid();
    if (eid < 0) {
        fprintf(stderr, "[EVENT] No more EIDs available\n");
        return -1;
    }
    
    ev->eid = eid;
    
    // Criar directoria do evento
    if (!create_event_directory(eid)) {
        fprintf(stderr, "[EVENT] Failed to create event directory for EID=%03d\n", eid);
        return -1;
    }
    
    // Criar ficheiro START_eid.txt
    char start_filename[64];
    snprintf(start_filename, sizeof(start_filename), "EVENTS/%03d/START_%03d.txt", eid, eid);
    
    FILE *fp = fopen(start_filename, "w");
    if (!fp) {
        fprintf(stderr, "[EVENT] Failed to create START file for EID=%03d\n", eid);
        return -1;
    }
    
    // Escrever: UID name desc_fname total_seats start_date start_time
    // Nota: o enunciado usa dd-mm-yyyy HH:MM mas vou seguir o protocolo dd-mm-yyyy
    fprintf(fp, "%s %s %s %d %s 00:00\n",
            ev->uid, ev->name, ev->filename, ev->total_seats, ev->date);
    fclose(fp);
    
    // Criar ficheiro RES_eid.txt com valor 0
    char res_filename[64];
    snprintf(res_filename, sizeof(res_filename), "EVENTS/%03d/RES_%03d.txt", eid, eid);
    
    fp = fopen(res_filename, "w");
    if (!fp) {
        fprintf(stderr, "[EVENT] Failed to create RES file for EID=%03d\n", eid);
        return -1;
    }
    fprintf(fp, "0\n");
    fclose(fp);
    
    // Guardar ficheiro de descrição em EVENTS/eid/DESCRIPTION/
    if (ev->filedata && ev->file_size > 0) {
        char desc_path[128];
        snprintf(desc_path, sizeof(desc_path), "EVENTS/%03d/DESCRIPTION/%s", eid, ev->filename);
        
        fp = fopen(desc_path, "wb");
        if (!fp) {
            fprintf(stderr, "[EVENT] Failed to save description file for EID=%03d\n", eid);
            return -1;
        }
        
        fwrite(ev->filedata, 1, ev->file_size, fp);
        fclose(fp);
    }
    
    // Criar ficheiro de referência em USERS/uid/CREATED/eid.txt
    char created_ref[64];
    snprintf(created_ref, sizeof(created_ref), "USERS/%s/CREATED/%03d.txt", ev->uid, eid);
    
    fp = fopen(created_ref, "w");
    if (fp) {
        fprintf(fp, "Event %03d created\n", eid);
        fclose(fp);
    }
    
    printf("[EVENT] Created event EID=%03d by UID=%s\n", eid, ev->uid);
    return 0;
}

/**
 * Lê informação de um evento
 */
int get_event(int eid, Event *ev) {
    if (!ev || eid < 1 || eid > 999) return -1;
    
    memset(ev, 0, sizeof(Event));
    ev->eid = eid;
    
    // Verificar se evento existe
    if (!event_exists(eid)) {
        return -1;
    }
    
    // Ler ficheiro START
    char start_filename[64];
    snprintf(start_filename, sizeof(start_filename), "EVENTS/%03d/START_%03d.txt", eid, eid);
    
    FILE *fp = fopen(start_filename, "r");
    if (!fp) {
        return -1;
    }
    
    char time_str[16];
    if (fscanf(fp, "%6s %10s %24s %d %10s %15s",
               ev->uid, ev->name, ev->filename, &ev->total_seats, ev->date, time_str) != 6) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    
    // Ler número de reservas de RES
    char res_filename[64];
    snprintf(res_filename, sizeof(res_filename), "EVENTS/%03d/RES_%03d.txt", eid, eid);
    
    fp = fopen(res_filename, "r");
    if (fp) {
        fscanf(fp, "%d", &ev->reserved_seats);
        fclose(fp);
    } else {
        ev->reserved_seats = 0;
    }
    
    // Verificar se existe ficheiro END
    char end_filename[64];
    snprintf(end_filename, sizeof(end_filename), "EVENTS/%03d/END_%03d.txt", eid, eid);
    struct stat st;
    if (stat(end_filename, &st) == 0) {
        ev->state = 3; // Fechado
    } else {
        ev->state = 1; // Ativo
    }
    
    return 0;
}

/**
 * Verifica se utilizador é dono do evento
 */
int is_event_owner(const char *uid, int eid) {
    if (!uid || eid < 1 || eid > 999) return 0;
    
    Event ev;
    if (get_event(eid, &ev) != 0) {
        return 0; // Evento não existe
    }
    
    return (strcmp(ev.uid, uid) == 0) ? 1 : 0;
}

/**
 * Verifica se data (dd-mm-yyyy HH:MM) já passou
 * Retorna: 1 se passou, 0 se não passou, -1 em erro
 */
int is_date_in_past(const char *event_date) {
    if (!event_date) return -1;
    
    // O protocolo usa dd-mm-yyyy, mas o guia menciona dd-mm-yyyy HH:MM
    // Vou assumir que event_date pode vir como "dd-mm-yyyy" sem hora
    // e compararei apenas a data, assumindo 00:00 para o evento
    
    int day, month, year;
    if (sscanf(event_date, "%d-%d-%d", &day, &month, &year) != 3) {
        fprintf(stderr, "[EVENT] Invalid date format: %s\n", event_date);
        return -1;
    }
    
    // Criar timestamp do evento (às 00:00)
    struct tm event_tm;
    memset(&event_tm, 0, sizeof(struct tm));
    event_tm.tm_mday = day;
    event_tm.tm_mon = month - 1;
    event_tm.tm_year = year - 1900;
    event_tm.tm_hour = 0;
    event_tm.tm_min = 0;
    event_tm.tm_sec = 0;
    
    time_t event_time = mktime(&event_tm);
    if (event_time == -1) {
        fprintf(stderr, "[EVENT] mktime failed for date: %s\n", event_date);
        return -1;
    }
    
    // Obter tempo atual
    time_t now = time(NULL);
    
    // Comparar apenas as datas (ignorar horas)
    struct tm *now_tm = localtime(&now);
    now_tm->tm_hour = 0;
    now_tm->tm_min = 0;
    now_tm->tm_sec = 0;
    time_t now_date = mktime(now_tm);
    
    return (event_time < now_date) ? 1 : 0;
}

/**
 * Obtém o estado de um evento
 * Retorna: 0=passado, 1=ativo, 2=sold-out, 3=fechado, -1=erro
 */
int get_event_state(int eid) {
    Event ev;
    
    if (get_event(eid, &ev) != 0) {
        return -1; // Evento não existe
    }
    
    // Verificar se tem ficheiro END (foi fechado manualmente)
    char end_filename[64];
    snprintf(end_filename, sizeof(end_filename), "EVENTS/%03d/END_%03d.txt", eid, eid);
    struct stat st;
    if (stat(end_filename, &st) == 0) {
        return 3; // Fechado
    }
    
    // Verificar se data já passou
    if (is_date_in_past(ev.date) == 1) {
        return 0; // Passado
    }
    
    // Verificar se está sold-out
    if (ev.reserved_seats >= ev.total_seats) {
        return 2; // Sold-out
    }
    
    return 1; // Ativo
}

/**
 * Fecha um evento
 * Cria ficheiro END_eid.txt com timestamp
 * Retorna: 0=OK, -1=não existe, -2=já fechado, -3=não é dono, 1=PST, 2=SLD
 */
int close_event(const char *uid, int eid) {
    if (!uid || eid < 1 || eid > 999) return -1;
    
    // Verificar se evento existe
    if (!event_exists(eid)) {
        return -1; // NOE
    }
    
    // Verificar ownership
    if (!is_event_owner(uid, eid)) {
        return -3; // EOW
    }
    
    // Verificar estado atual
    int state = get_event_state(eid);
    
    if (state == 3) {
        return -2; // CLO - já fechado
    }
    
    if (state == 0) {
        return 1; // PST - já passou
    }
    
    if (state == 2) {
        return 2; // SLD - sold-out
    }
    
    // Pode fechar - criar ficheiro END
    char end_filename[64];
    snprintf(end_filename, sizeof(end_filename), "EVENTS/%03d/END_%03d.txt", eid, eid);
    
    FILE *fp = fopen(end_filename, "w");
    if (!fp) {
        fprintf(stderr, "[EVENT] Failed to create END file for EID=%03d\n", eid);
        return -1;
    }
    
    // Escrever timestamp de encerramento
    char timestamp[32];
    get_current_timestamp(timestamp, sizeof(timestamp));
    fprintf(fp, "%s\n", timestamp);
    fclose(fp);
    
    printf("[EVENT] Closed event EID=%03d by UID=%s at %s\n", eid, uid, timestamp);
    return 0; // OK
}
