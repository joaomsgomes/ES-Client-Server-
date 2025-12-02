#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include "../../include/es_server.h"
#include "../../include/utils.h"

/**
 * Inicializa o sistema de ficheiros do servidor
 * Cria as directorias USERS/ e EVENTS/ se não existirem
 */
void init_file_system() {
    // Criar directoria USERS
    if (mkdir("USERS", 0700) == -1) {
        // Já existe ou erro
        struct stat st;
        if (stat("USERS", &st) == 0 && S_ISDIR(st.st_mode)) {
            printf("[INIT] Directory USERS/ already exists\n");
        } else {
            perror("[INIT] Error creating USERS directory");
            exit(1);
        }
    } else {
        printf("[INIT] Created directory USERS/\n");
    }
    
    // Criar directoria EVENTS
    if (mkdir("EVENTS", 0700) == -1) {
        struct stat st;
        if (stat("EVENTS", &st) == 0 && S_ISDIR(st.st_mode)) {
            printf("[INIT] Directory EVENTS/ already exists\n");
        } else {
            perror("[INIT] Error creating EVENTS directory");
            exit(1);
        }
    } else {
        printf("[INIT] Created directory EVENTS/\n");
    }
    
    printf("[INIT] File system initialized\n");
}

/**
 * Cria a directoria de um utilizador e subdirectorias
 * USERS/uid/
 * USERS/uid/CREATED/
 * USERS/uid/RESERVED/
 */
int create_user_directory(const char *uid) {
    char dirname[64];
    
    // Criar USERS/uid/
    snprintf(dirname, sizeof(dirname), "USERS/%s", uid);
    if (mkdir(dirname, 0700) == -1) {
        perror("[FS] Error creating user directory");
        return 0;
    }
    
    // Criar USERS/uid/CREATED/
    snprintf(dirname, sizeof(dirname), "USERS/%s/CREATED", uid);
    if (mkdir(dirname, 0700) == -1) {
        perror("[FS] Error creating CREATED directory");
        return 0;
    }
    
    // Criar USERS/uid/RESERVED/
    snprintf(dirname, sizeof(dirname), "USERS/%s/RESERVED", uid);
    if (mkdir(dirname, 0700) == -1) {
        perror("[FS] Error creating RESERVED directory");
        return 0;
    }
    
    return 1;
}

/**
 * Cria a directoria de um evento e subdirectorias
 * EVENTS/eid/
 * EVENTS/eid/DESCRIPTION/
 * EVENTS/eid/RESERVATIONS/
 */
int create_event_directory(int eid) {
    char eid_dirname[32];
    char reserv_dirname[64];
    char desc_dirname[64];
    int ret;
    
    if (eid < 1 || eid > 999) {
        return 0;
    }
    
    // Criar EVENTS/eid/
    snprintf(eid_dirname, sizeof(eid_dirname), "EVENTS/%03d", eid);
    ret = mkdir(eid_dirname, 0700);
    if (ret == -1) {
        return 0;
    }
    
    // Criar EVENTS/eid/RESERVATIONS/
    snprintf(reserv_dirname, sizeof(reserv_dirname), "EVENTS/%03d/RESERVATIONS", eid);
    ret = mkdir(reserv_dirname, 0700);
    if (ret == -1) {
        rmdir(eid_dirname);
        return 0;
    }
    
    // Criar EVENTS/eid/DESCRIPTION/
    snprintf(desc_dirname, sizeof(desc_dirname), "EVENTS/%03d/DESCRIPTION", eid);
    ret = mkdir(desc_dirname, 0700);
    if (ret == -1) {
        rmdir(eid_dirname);
        rmdir(reserv_dirname);
        return 0;
    }
    
    return 1;
}

/**
 * Cria ficheiro de login (uid_login.txt)
 */
int create_login_file(const char *uid) {
    char login_name[64];
    FILE *fp;
    
    if (strlen(uid) != 6) {
        return 0;
    }
    
    snprintf(login_name, sizeof(login_name), "USERS/%s/%s_login.txt", uid, uid);
    fp = fopen(login_name, "w");
    if (fp == NULL) {
        return 0;
    }
    
    fprintf(fp, "Logged in\n");
    fclose(fp);
    return 1;
}

/**
 * Remove ficheiro de login
 */
int erase_login_file(const char *uid) {
    char login_name[64];
    
    if (strlen(uid) != 6) {
        return 0;
    }
    
    snprintf(login_name, sizeof(login_name), "USERS/%s/%s_login.txt", uid, uid);
    unlink(login_name);
    return 1;
}

/**
 * Verifica se utilizador está logged in
 */
int is_logged_in(const char *uid) {
    char login_name[64];
    struct stat st;
    
    snprintf(login_name, sizeof(login_name), "USERS/%s/%s_login.txt", uid, uid);
    return (stat(login_name, &st) == 0);
}

/**
 * Cria/atualiza ficheiro de password
 */
int write_password_file(const char *uid, const char *password) {
    char pass_name[64];
    FILE *fp;
    
    if (strlen(uid) != 6 || strlen(password) != 8) {
        return 0;
    }
    
    snprintf(pass_name, sizeof(pass_name), "USERS/%s/%s_pass.txt", uid, uid);
    fp = fopen(pass_name, "w");
    if (fp == NULL) {
        return 0;
    }
    
    fprintf(fp, "%s\n", password);
    fclose(fp);
    return 1;
}

/**
 * Lê password do ficheiro
 */
int read_password_file(const char *uid, char *password) {
    char pass_name[64];
    FILE *fp;
    
    if (strlen(uid) != 6) {
        return 0;
    }
    
    snprintf(pass_name, sizeof(pass_name), "USERS/%s/%s_pass.txt", uid, uid);
    fp = fopen(pass_name, "r");
    if (fp == NULL) {
        return 0;
    }
    
    if (fscanf(fp, "%8s", password) != 1) {
        fclose(fp);
        return 0;
    }
    
    fclose(fp);
    return 1;
}

/**
 * Remove ficheiro de password
 */
int erase_password_file(const char *uid) {
    char pass_name[64];
    
    if (strlen(uid) != 6) {
        return 0;
    }
    
    snprintf(pass_name, sizeof(pass_name), "USERS/%s/%s_pass.txt", uid, uid);
    unlink(pass_name);
    return 1;
}

/**
 * Obtem próximo EID disponível
 * Procura em EVENTS/ qual o maior número de directoria
 */
int get_next_eid() {
    struct dirent **filelist;
    int nentries, ient = 0;
    int max_eid = 0;
    
    nentries = scandir("EVENTS", &filelist, 0, alphasort);
    
    if (nentries <= 0) {
        return 1; // Primeiro evento
    }
    
    while (ient < nentries) {
        if (filelist[ient]->d_name[0] != '.') {
            int eid = atoi(filelist[ient]->d_name);
            if (eid > max_eid && eid < 1000) {
                max_eid = eid;
            }
        }
        free(filelist[ient]);
        ient++;
    }
    free(filelist);
    
    if (max_eid >= 999) {
        return -1; // Base de dados cheia
    }
    
    return max_eid + 1;
}

/**
 * Verifica se um evento existe
 */
int event_exists(int eid) {
    char dirname[32];
    struct stat st;
    
    snprintf(dirname, sizeof(dirname), "EVENTS/%03d", eid);
    return (stat(dirname, &st) == 0 && S_ISDIR(st.st_mode));
}

/**
 * Verifica se utilizador existe (tem directoria)
 */
int user_exists(const char *uid) {
    char dirname[32];
    struct stat st;
    
    snprintf(dirname, sizeof(dirname), "USERS/%s", uid);
    return (stat(dirname, &st) == 0 && S_ISDIR(st.st_mode));
}

/**
 * Converte data dd-mm-yyyy HH:MM para Unix timestamp
 * Retorna -1 em caso de erro
 */
time_t date_to_timestamp(const char *date_str) {
    struct tm datetime;
    int day, month, year, hour, min;
    
    // Parse: dd-mm-yyyy HH:MM
    if (sscanf(date_str, "%d-%d-%d %d:%d", &day, &month, &year, &hour, &min) != 5) {
        return -1;
    }
    
    memset(&datetime, 0, sizeof(struct tm));
    datetime.tm_mday = day;
    datetime.tm_mon = month - 1;
    datetime.tm_year = year - 1900;
    datetime.tm_hour = hour;
    datetime.tm_min = min;
    datetime.tm_sec = 0;
    
    return mktime(&datetime);
}

/**
 * Obtem timestamp atual formatado como DD-MM-YYYY HH:MM:SS
 */
void get_current_timestamp(char *buffer, size_t size) {
    time_t fulltime;
    struct tm *current_time;
    
    time(&fulltime);
    current_time = gmtime(&fulltime);
    
    snprintf(buffer, size, "%02d-%02d-%04d %02d:%02d:%02d",
             current_time->tm_mday, current_time->tm_mon + 1, current_time->tm_year + 1900,
             current_time->tm_hour, current_time->tm_min, current_time->tm_sec);
}
