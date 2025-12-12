#ifndef USER_MANAGEMENT_H
#define USER_MANAGEMENT_H

#include <stdbool.h>

/**
 * User Management Module
 * Handles user registration, authentication, sessions and data persistence
 * 
 * Implementado em: user_database.c
 */


// ===== AUTENTICAÇÃO =====
bool register_user(const char* uid, const char* password);
int authenticate_user(const char* uid, const char* password);
/**
 * authenticate_user return values:
 *   1 = authentication successful
 *   0 = user does not exist
 *  -1 = incorrect password
 */

// ===== GESTÃO DE SESSÕES =====
bool login_user(const char* uid);
bool logout_user(const char* uid);
bool is_user_logged_in(const char* uid);

// ===== GESTÃO DE CONTAS =====
bool unregister_user(const char* uid);
bool change_password(const char* uid, const char* old_password, const char* new_password);

#endif // USER_MANAGEMENT_H