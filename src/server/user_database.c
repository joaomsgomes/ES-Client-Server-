#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sqlite3.h>
#include "../../include/es_server.h"
#include "../../include/utils.h"

#define DB_FILE "es_server.db"

static sqlite3 *db = NULL;

/**
 * Inicializa o sistema de gestão de utilizadores com SQLite
 * Cria a base de dados e tabela se não existirem
 */
void init_user_system() {
    int rc;
    char *err_msg = NULL;
    
    // Abrir/criar base de dados
    rc = sqlite3_open(DB_FILE, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }
    
    // Criar tabela users se não existir
    const char *sql_create_table = 
        "CREATE TABLE IF NOT EXISTS users ("
        "   uid TEXT PRIMARY KEY NOT NULL,"
        "   password TEXT NOT NULL,"
        "   created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";
    
    rc = sqlite3_exec(db, sql_create_table, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        exit(1);
    }
    
    printf("[INIT] User system initialized with SQLite (%s)\n", DB_FILE);
}

/**
 * Verifica se um utilizador existe na base de dados
 */
static bool user_exists(const char* uid) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT 1 FROM users WHERE uid = ? LIMIT 1;";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, uid, -1, SQLITE_STATIC);
    
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    
    sqlite3_finalize(stmt);
    return exists;
}

/**
 * Carrega a password de um utilizador da base de dados
 * Retorna true se conseguiu carregar, false caso contrário
 */
static bool load_password(const char* uid, char* password_out) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT password FROM users WHERE uid = ?;";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, uid, -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *password = sqlite3_column_text(stmt, 0);
        strncpy(password_out, (const char*)password, PASSWORD_LEN);
        password_out[PASSWORD_LEN] = '\0';
        sqlite3_finalize(stmt);
        return true;
    }
    
    sqlite3_finalize(stmt);
    return false;
}

/**
 * Guarda a password de um utilizador na base de dados
 */
static bool save_password(const char* uid, const char* password) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO users (uid, password) VALUES (?, ?);";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, uid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert user: %s\n", sqlite3_errmsg(db));
        return false;
    }
    
    return true;
}

/**
 * Regista um novo utilizador
 * Retorna true se registado com sucesso
 */
bool register_user(const char* uid, const char* password) {
    if (!validate_uid(uid) || !validate_password(password)) {
        return false;
    }
    
    if (user_exists(uid)) {
        return false; // Já existe
    }
    
    if (!save_password(uid, password)) {
        return false;
    }
    
    printf("[USER] Registered new user: %s\n", uid);
    return true;
}

/**
 * Autentica um utilizador (verifica se UID existe e password está correta)
 * Retorna:
 *   1 = autenticação bem-sucedida
 *   0 = utilizador não existe
 *  -1 = password incorreta
 */
int authenticate_user(const char* uid, const char* password) {
    if (!validate_uid(uid) || !validate_password(password)) {
        return -1;
    }
    
    if (!user_exists(uid)) {
        return 0; // Utilizador não existe
    }
    
    char stored_password[PASSWORD_LEN + 1];
    if (!load_password(uid, stored_password)) {
        return -1;
    }
    
    if (strcmp(stored_password, password) == 0) {
        return 1; // Password correta
    }
    
    return -1; // Password incorreta
}

/**
 * Cleanup: fecha a conexão com a base de dados
 * Chamar antes de terminar o servidor
 */
void cleanup_user_system() {
    if (db != NULL) {
        sqlite3_close(db);
        db = NULL;
        printf("[CLEANUP] Database connection closed\n");
    }
}
