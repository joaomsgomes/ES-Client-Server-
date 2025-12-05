#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "../../include/es_server.h"
#include "../../include/file_system.h"
#include "../../include/utils.h"


/**
 * Regista um novo utilizador
 * Cria: USERS/uid/, uid_pass.txt, CREATED/, RESERVED/
 * Retorna true se registado com sucesso
 */
bool register_user(const char* uid, const char* password) {
    if (!validate_uid(uid) || !validate_password(password)) {
        fprintf(stderr, "[USER] Invalid UID or password format\n");
        return false;
    }
    
    // Verificar se utilizador já existe
    if (user_exists(uid)) {
        // Se já existe directoria mas não tem password, é um re-registo
        char test_pass[PASSWORD_LEN + 1];
        if (read_password_file(uid, test_pass)) {
            fprintf(stderr, "[USER] User %s already registered\n", uid);
            return false;
        }
        // Se chegou aqui, existe directoria mas sem password (foi unregister)
        // Pode re-registar com nova password
    } else {
        // Criar directoria do utilizador
        if (!create_user_directory(uid)) {
            fprintf(stderr, "[USER] Failed to create user directory for %s\n", uid);
            return false;
        }
    }
    
    // Criar/atualizar ficheiro de password
    if (!write_password_file(uid, password)) {
        fprintf(stderr, "[USER] Failed to write password file for %s\n", uid);
        return false;
    }
    
    printf("[USER] Registered user: %s\n", uid);
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
    
    // Verificar se utilizador existe (tem ficheiro de password)
    char stored_password[PASSWORD_LEN + 1];
    if (!read_password_file(uid, stored_password)) {
        return 0; // Utilizador não existe ou não tem password
    }
    
    // Verificar password
    if (strcmp(password, stored_password) != 0) {
        return -1; // Password incorreta
    }
    
    return 1; // Autenticação bem-sucedida
}

/**
 * Marca um utilizador como logged in
 * Cria ficheiro uid_login.txt
 */
bool login_user(const char* uid) {
    if (!validate_uid(uid)) {
        return false;
    }
    
    if (!create_login_file(uid)) {
        fprintf(stderr, "[USER] Failed to create login file for %s\n", uid);
        return false;
    }
    
    printf("[USER] User logged in: %s\n", uid);
    return true;
}

/**
 * Marca um utilizador como logged out
 * Remove ficheiro uid_login.txt
 */
bool logout_user(const char* uid) {
    if (!validate_uid(uid)) {
        return false;
    }
    
    if (!erase_login_file(uid)) {
        fprintf(stderr, "[USER] Failed to erase login file for %s\n", uid);
        return false;
    }
    
    printf("[USER] User logged out: %s\n", uid);
    return true;
}

/**
 * Verifica se um utilizador está logged in
 */
bool is_user_logged_in(const char* uid) {
    return is_logged_in(uid);
}

/**
 * Remove registo de um utilizador
 * Remove ficheiros uid_pass.txt e uid_login.txt
 * Preserva directorias CREATED/ e RESERVED/
 */
bool unregister_user(const char* uid) {
    if (!validate_uid(uid)) {
        return false;
    }
    
    // Verificar se utilizador existe
    if (!user_exists(uid)) {
        fprintf(stderr, "[USER] User %s does not exist\n", uid);
        return false;
    }
    
    // Remover ficheiro de password
    erase_password_file(uid);
    
    // Remover ficheiro de login se existir
    erase_login_file(uid);
    
    printf("[USER] Unregistered user: %s (directories preserved)\n", uid);
    return true;
}

/**
 * Altera a password de um utilizador
 */
bool change_password(const char* uid, const char* old_password, const char* new_password) {
    if (!validate_uid(uid) || !validate_password(old_password) || !validate_password(new_password)) {
        return false;
    }
    
    // Autenticar com password antiga
    int auth_result = authenticate_user(uid, old_password);
    if (auth_result != 1) {
        return false;
    }
    
    // Gravar nova password
    if (!write_password_file(uid, new_password)) {
        fprintf(stderr, "[USER] Failed to update password for %s\n", uid);
        return false;
    }
    
    printf("[USER] Password changed for user: %s\n", uid);
    return true;
}
