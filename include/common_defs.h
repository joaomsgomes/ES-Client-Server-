#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

// ============ CONSTANTES PARTILHADAS (Cliente + Servidor) ============

// Formatos e tamanhos de dados
#define PASSWORD_LEN 8
#define UID_LEN 6
#define EVENT_NAME_LEN 10
#define DATE_STR_LEN 10 // dd-mm-yyyy
#define TIME_STR_LEN 5  // HH:MM
#define TIME_WITH_SECONDS_LEN 8  // HH:MM:SS
#define TIME_BUFFER_LEN 6  // HH:MM\0
#define TIME_WITH_SECONDS_BUFFER_LEN 9  // HH:MM:SS\0
#define DATETIME_BUFFER_LEN 20  // dd-mm-yyyy HH:MM\0
#define DATETIME_WITH_SECONDS_LEN 20  // dd-mm-yyyy HH:MM:SS\0
#define FILENAME_LEN 24
#define MAX_FILE_SIZE 10000000 // 10 MB

// Tamanhos de buffers comuns
#define SMALL_BUFFER 32
#define RESPONSE_BUFFER 64
#define PATH_BUFFER 128
#define MEDIUM_BUFFER 256
#define FILE_PATH_BUFFER 512
#define LARGE_BUFFER 4096
#define XLARGE_BUFFER 8192

// Protocolo
#define CMD_LEN 4  // 3 chars + \0
#define STATUS_LEN 4  // 3 chars + \0
#define EID_STR_LEN 4  // 3 digits + \0

// Limites de listas
#define MAX_RESERVATIONS_QUERY 50

// Paths do sistema de ficheiros
#define USERS_DIR "USERS/"
#define EVENTS_DIR "EVENTS/"
#define CREATED_SUBDIR "CREATED/"
#define RESERVED_SUBDIR "RESERVED/"
#define RESERVATIONS_SUBDIR "RESERVATIONS/"
#define DESCRIPTION_SUBDIR "DESCRIPTION/"

// Overhead TCP
#define TCP_BUFFER_OVERHEAD 1024

#endif
