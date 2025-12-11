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
    fprintf(fp, "%s %s %s %d %s %s\n",
            ev->uid, ev->name, ev->filename, ev->total_seats, ev->date, ev->time);
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
    
    if (fscanf(fp, "%6s %10s %24s %d %10s %5s",
               ev->uid, ev->name, ev->filename, &ev->total_seats, ev->date, ev->time) != 6) {
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
    
    // Carregar ficheiro de descrição
    char desc_path[128];
    snprintf(desc_path, sizeof(desc_path), "EVENTS/%03d/DESCRIPTION/%s", eid, ev->filename);
    
    fp = fopen(desc_path, "rb");
    if (!fp) {
        ev->file_size = 0;
        ev->filedata = NULL;
        return -1; // Ficheiro obrigatório para show
    }
    
    // Obter tamanho do ficheiro
    fseek(fp, 0, SEEK_END);
    ev->file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (ev->file_size <= 0) {
        fclose(fp);
        ev->filedata = NULL;
        ev->file_size = 0;
        return -1;
    }
    
    // Alocar e ler dados
    ev->filedata = malloc(ev->file_size);
    if (!ev->filedata) {
        fclose(fp);
        ev->file_size = 0;
        return -1;
    }
    
    size_t read_bytes = fread(ev->filedata, 1, ev->file_size, fp);
    fclose(fp);
    
    if (read_bytes != (size_t)ev->file_size) {
        free(ev->filedata);
        ev->filedata = NULL;
        ev->file_size = 0;
        return -1;
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
 * Obtém o estado de um evento
 * Retorna: 0=passado, 1=ativo, 2=sold-out, 3=fechado, -1=erro
 */
int get_event_state(int eid) {
    if (!event_exists(eid)) return -1;
    
    // Verificar se tem ficheiro END (foi fechado manualmente)
    char end_filename[64];
    snprintf(end_filename, sizeof(end_filename), "EVENTS/%03d/END_%03d.txt", eid, eid);
    struct stat st;
    if (stat(end_filename, &st) == 0) {
        return 3; // Fechado
    }
    
    // Ler data do evento
    char start_file[128];
    snprintf(start_file, sizeof(start_file), "EVENTS/%03d/START_%03d.txt", eid, eid);
    
    FILE *fp = fopen(start_file, "r");
    if (!fp) return -1;
    
    char uid[7], name[11], fname[25], date[11], event_time_str[6];
    int total_seats;
    
    if (fscanf(fp, "%6s %10s %24s %d %10s %5s", uid, name, fname, &total_seats, date, event_time_str) != 6) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    
    // Adicionar hora à data para comparação: "dd-mm-yyyy HH:MM"
    char full_date[32];
    snprintf(full_date, sizeof(full_date), "%s %s", date, event_time_str);
    
    // Verificar se data já passou
    time_t event_timestamp = date_to_timestamp(full_date);
    time_t now_timestamp = time(NULL);
    
    if (event_timestamp < now_timestamp) {
        return 0; // Passado
    }
    
    // Ler total de reservas
    int total, reserved;
    if (!get_event_seats(eid, &total, &reserved)) {
        return -1;
    }
    
    // Verificar se está sold-out
    if (reserved >= total) {
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

/**
 * Lê total de lugares e lugares reservados de um evento
 * Retorna: 1=sucesso, 0=erro
 */
int get_event_seats(int eid, int *total_seats, int *reserved_seats) {
    // Ler total_seats de START_eid.txt
    char start_file[128];
    snprintf(start_file, sizeof(start_file), "EVENTS/%03d/START_%03d.txt", eid, eid);
    
    FILE *fp = fopen(start_file, "r");
    if (!fp) return 0;
    
    char uid[7], name[11], fname[25], date[11], time[6];
    
    int fields = fscanf(fp, "%6s %10s %24s %d %10s %5s",
                       uid, name, fname, total_seats, date, time);
    fclose(fp);
    
    if (fields < 6) return 0;
    
    // Ler reserved_seats de RES_eid.txt
    char res_file[128];
    snprintf(res_file, sizeof(res_file), "EVENTS/%03d/RES_%03d.txt", eid, eid);
    
    fp = fopen(res_file, "r");
    if (!fp) {
        *reserved_seats = 0;  // Ficheiro não existe = 0 reservas
        return 1;
    }
    
    if (fscanf(fp, "%d", reserved_seats) != 1) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    
    return 1;
}


int create_reservation(int eid, const char *uid, int num_seats) {

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H%M%S", tm_info);
    
    char filename[128];
    snprintf(filename, sizeof(filename), "R-%s-%s.txt", uid, timestamp);
    
    char content[128];
    char display_timestamp[32];
    strftime(display_timestamp, sizeof(display_timestamp), "%d-%m-%Y %H:%M:%S", tm_info);
    snprintf(content, sizeof(content), "%s %d %s\n", uid, num_seats, display_timestamp);
    
    char event_path[256];
    snprintf(event_path, sizeof(event_path), "EVENTS/%03d/RESERVATIONS/%s", eid, filename);
    
    FILE *fp = fopen(event_path, "w");
    if (!fp) {printf(" !fp\n"); return 0;}
    
    fprintf(fp, "%s", content);
    fclose(fp);
    
    char user_path[256];
    snprintf(user_path, sizeof(user_path), "USERS/%s/RESERVED/%s", uid, filename);
    
    fp = fopen(user_path, "w");
    if (fp) {
        fprintf(fp, "%s", content);
        fclose(fp);
    }
    
    char res_file[128];
    snprintf(res_file, sizeof(res_file), "EVENTS/%03d/RES_%03d.txt", eid, eid);
    
    int current_reserved = 0;
    fp = fopen(res_file, "r");
    if (fp) {
        fscanf(fp, "%d", &current_reserved);
        fclose(fp);
    }
    
    fp = fopen(res_file, "w");
    if (!fp) {printf("!fp\n"); return 0;}
    
    fprintf(fp, "%d\n", current_reserved + num_seats);
    fclose(fp);
    
    printf("[DB] Created reservation: %s (EID=%03d, seats=%d)\n", filename, eid, num_seats);
    return 1;
}