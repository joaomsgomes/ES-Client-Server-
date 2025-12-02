#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sqlite3.h>
#include "../../include/es_server.h"
#include "../../include/utils.h"

static sqlite3 *db = NULL;


void init_event_system() {
    int rc;
    char *err_msg = NULL;
    
    // Abrir/criar base de dados
    rc = sqlite3_open(DB_FILE, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }
    
    // Criar tabela events se não existir
    const char *sql_create_table =
        "CREATE TABLE IF NOT EXISTS events ("
        "  eid INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  uid TEXT NOT NULL,"
        "  name TEXT NOT NULL,"
        "  date TEXT NOT NULL,"          
        "  total_seats INTEGER NOT NULL,"
        "  reserved_seats INTEGER DEFAULT 0,"
        "  filename TEXT NOT NULL,"
        "  file_size INTEGER NOT NULL,"
        "  filedata BLOB,"
        "  state INTEGER DEFAULT 1,"           
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY(uid) REFERENCES users(uid)"
        ");";
    
    rc = sqlite3_exec(db, sql_create_table, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        exit(1);
    }
    
    printf("[INIT] Event system initialized with SQLite (%s)\n", DB_FILE);
}


int create_event(sqlite3 *db, Event *ev) {
    if (!db || !ev) return SQLITE_MISUSE;

    int rc;
    sqlite3_stmt *stmt = NULL;

    const char *sql =
        "INSERT INTO events ("
        "  uid, name, date, total_seats,"
        "  reserved_seats, filename, file_size, filedata, state"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DB] prepare INSERT events failed: %s\n",
                sqlite3_errmsg(db));
        return rc;
    }

    // 1: uid
    rc = sqlite3_bind_text(stmt, 1, ev->uid, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) goto bind_error;

    // 2: name
    rc = sqlite3_bind_text(stmt, 2, ev->name, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) goto bind_error;

    // 3: date (string "dd-mm-yyyy")
    rc = sqlite3_bind_text(stmt, 3, ev->date, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) goto bind_error;

    // 4: total_seats
    rc = sqlite3_bind_int(stmt, 4, ev->total_seats);
    if (rc != SQLITE_OK) goto bind_error;

    // 5: reserved_seats
    rc = sqlite3_bind_int(stmt, 5, ev->reserved_seats);
    if (rc != SQLITE_OK) goto bind_error;

    // 6: filename
    rc = sqlite3_bind_text(stmt, 6, ev->filename, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) goto bind_error;

    // 7: file_size
    rc = sqlite3_bind_int64(stmt, 7, (sqlite3_int64)ev->file_size);
    if (rc != SQLITE_OK) goto bind_error;

    // 8: filedata (pode ser NULL)
    if (ev->filedata != NULL && ev->file_size > 0) {
        rc = sqlite3_bind_blob(stmt, 8, ev->filedata,
                               (int)ev->file_size, SQLITE_TRANSIENT);
    } else {
        rc = sqlite3_bind_null(stmt, 8);
    }
    if (rc != SQLITE_OK) goto bind_error;

    // 9: state
    rc = sqlite3_bind_int(stmt, 9, ev->state);
    if (rc != SQLITE_OK) goto bind_error;

    // Executar
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "[DB] insert event failed: %s\n",
                sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return rc;
    }

    // Buscar eid gerado
    ev->eid = (int)sqlite3_last_insert_rowid(db);

    sqlite3_finalize(stmt);
    return SQLITE_OK;

bind_error:
    fprintf(stderr, "[DB] bind error in create_event: %s\n",
            sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return rc;
}


int get_event(sqlite3 *db, int eid, Event *ev) {
    if (!db || !ev) return SQLITE_MISUSE;

    int rc;
    sqlite3_stmt *stmt = NULL;

    
    memset(ev, 0, sizeof(*ev));
    ev->filedata = NULL;

    const char *sql =
        "SELECT eid, uid, name, event_date, "
        "       attendance_size, seats_reserved, "
        "       filename, filesize, filedata, state "
        "FROM events "
        "WHERE eid = ?;";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DB] prepare SELECT event failed: %s\n",
                sqlite3_errmsg(db));
        return rc;
    }

    // Bind do eid
    rc = sqlite3_bind_int(stmt, 1, eid);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DB] bind eid failed: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return rc;
    }

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_DONE) {
        // Não há linha com esse eid
        sqlite3_finalize(stmt);
        return SQLITE_NOTFOUND; 
    }

    if (rc != SQLITE_ROW) {
        // Erro de execução
        fprintf(stderr, "[DB] step SELECT event failed: %s\n",
                sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return rc;
    }

    

    // 0: eid
    ev->eid = sqlite3_column_int(stmt, 0);

    // 1: uid
    const unsigned char *uid_col = sqlite3_column_text(stmt, 1);
    snprintf(ev->uid, sizeof(ev->uid), "%s",
             uid_col ? (const char *)uid_col : "");

    // 2: name
    const unsigned char *name_col = sqlite3_column_text(stmt, 2);
    snprintf(ev->name, sizeof(ev->name), "%s",
             name_col ? (const char *)name_col : "");

    // 3: event_date (string)
    const unsigned char *date_col = sqlite3_column_text(stmt, 3);
    snprintf(ev->date, sizeof(ev->date), "%s",
             date_col ? (const char *)date_col : "");

    // 4: attendance_size → total_seats
    ev->total_seats = sqlite3_column_int(stmt, 4);

    // 5: seats_reserved
    ev->reserved_seats = sqlite3_column_int(stmt, 5);

    // 6: filename
    const unsigned char *fname_col = sqlite3_column_text(stmt, 6);
    snprintf(ev->filename, sizeof(ev->filename), "%s",
             fname_col ? (const char *)fname_col : "");

    // 7: filesize
    sqlite3_int64 fs = sqlite3_column_int64(stmt, 7);
    ev->file_size = (long)fs;

    // 8: filedata (BLOB) – pode ser NULL
    int blob_size = sqlite3_column_bytes(stmt, 8);
    const void *blob = sqlite3_column_blob(stmt, 8);

    if (blob && blob_size > 0) {
        ev->filedata = malloc(blob_size);
        if (!ev->filedata) {
            fprintf(stderr, "[DB] malloc for filedata failed\n");
            sqlite3_finalize(stmt);
            return SQLITE_NOMEM;
        }
        memcpy(ev->filedata, blob, blob_size);
        ev->file_size = blob_size;
    } else {
        ev->filedata = NULL;
        ev->file_size = 0;
    }

    // 9: state
    ev->state = sqlite3_column_int(stmt, 9);

    sqlite3_finalize(stmt);
    return SQLITE_OK;
}

int is_event_owner(sqlite3 *db, const char *uid, int eid) {
    if (!db || !uid) {
        fprintf(stderr, "[DB] is_event_owner: db or uid is NULL\n");
        return -1;
    }

    int rc;
    sqlite3_stmt *stmt = NULL;
    int result = 0; // por default, não é dono

    const char *sql =
        "SELECT 1 "
        "FROM events "
        "WHERE eid = ? AND uid = ? "
        "LIMIT 1;";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DB] prepare is_event_owner failed: %s\n",
                sqlite3_errmsg(db));
        return -1;
    }

    // Bind do eid (1º ?) e uid (2º ?)
    rc = sqlite3_bind_int(stmt, 1, eid);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DB] bind eid failed: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    rc = sqlite3_bind_text(stmt, 2, uid, -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DB] bind uid failed: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        // Há uma linha → existe evento com esse eid e esse uid
        result = 1;
    } else if (rc == SQLITE_DONE) {
        // Nenhuma linha → ou não existe evento, ou não pertence a este uid
        result = 0;
    } else {
        // Erro na execução
        fprintf(stderr, "[DB] step is_event_owner failed: %s\n",
                sqlite3_errmsg(db));
        result = -1;
    }

    sqlite3_finalize(stmt);
    return result;
}

