#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "../../include/es_server.h"
#include "../../include/user_management.h"
#include "../../include/utils.h"

#define USER_DATA_DIR "USERS"

/**
 * Inicializa o sistema de gestão de utilizadores
 * Cria o diretório USERS se não existir
 */

//REVER UTILIDADE DESTAS FUNÇÕES
 
void init_user_system() {
    struct stat st = {0};
    
    if (stat(USER_DATA_DIR, &st) == -1) {
        if (mkdir(USER_DATA_DIR, 0700) == -1) {
            perror("Error creating USERS directory");
            exit(1);
        }
    }
    printf("[INIT] User system initialized\n");
}

/**
 * Verifica se um utilizador existe (ficheiro USERS/UID_pass.txt existe)
 */
static bool user_exists(const char* uid) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s_pass.txt", USER_DATA_DIR, uid);
    
    FILE* file = fopen(filepath, "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

/**
 * Carrega a password de um utilizador do disco
 * Retorna true se conseguiu carregar, false caso contrário
 */
static bool load_password(const char* uid, char* password_out) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s_pass.txt", USER_DATA_DIR, uid);
    
    FILE* file = fopen(filepath, "r");
    if (!file) {
        return false;
    }
    
    // Lê exatamente PASSWORD_LEN caracteres
    if (fgets(password_out, PASSWORD_LEN + 2, file) == NULL) {
        fclose(file);
        return false;
    }
    
    // Remove \n se existir
    size_t len = strlen(password_out);
    if (len > 0 && password_out[len-1] == '\n') {
        password_out[len-1] = '\0';
    }
    
    fclose(file);
    return true;
}

/**
 * Guarda a password de um utilizador no disco
 */
static bool save_password(const char* uid, const char* password) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s_pass.txt", USER_DATA_DIR, uid);
    
    FILE* file = fopen(filepath, "w");
    if (!file) {
        perror("Error creating user file");
        return false;
    }
    
    fprintf(file, "%s\n", password);
    fclose(file);
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
