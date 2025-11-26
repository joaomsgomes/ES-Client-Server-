#ifndef USER_MANAGEMENT_H
#define USER_MANAGEMENT_H

#include <stdbool.h>

/**
 * User Management Module
 * Handles user registration, authentication and data persistence
 */

// Initialization
void init_user_system();

// User operations
bool register_user(const char* uid, const char* password);
int authenticate_user(const char* uid, const char* password);

/**
 * authenticate_user return values:
 *   1 = authentication successful
 *   0 = user does not exist
 *  -1 = incorrect password
 */

#endif // USER_MANAGEMENT_H
