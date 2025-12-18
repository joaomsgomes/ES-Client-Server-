#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "../../include/es_server.h"
#include "../../include/file_system.h"
#include "../../include/utils.h"



bool register_user(const char* uid, const char* password) {
    if (!validate_uid(uid) || !validate_password(password)) {
        fprintf(stderr, "[USER] Invalid UID or password format\n");
        return false;
    }
    
    if (user_exists(uid)) {
        
        char test_pass[PASSWORD_LEN + 1];
        if (read_password_file(uid, test_pass)) {
            fprintf(stderr, "[USER] User %s already registered\n", uid);
            return false;
        }
        
    } else {
        
        if (!create_user_directory(uid)) {
            fprintf(stderr, "[USER] Failed to create user directory for %s\n", uid);
            return false;
        }
    }
    
    
    if (!write_password_file(uid, password)) {
        fprintf(stderr, "[USER] Failed to write password file for %s\n", uid);
        return false;
    }
    
    printf("[USER] Registered user: %s\n", uid);
    return true;
}


int authenticate_user(const char* uid, const char* password) {
    if (!validate_uid(uid) || !validate_password(password)) {
        return -1;
    }
    
    char stored_password[PASSWORD_LEN + 1];
    if (!read_password_file(uid, stored_password)) {
        return 0;
    }
    
    
    if (strcmp(password, stored_password) != 0) {
        return -1;
    }
    
    return 1;
}


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

bool is_user_logged_in(const char* uid) {
    return is_logged_in(uid);
}


bool unregister_user(const char* uid) {
    if (!validate_uid(uid)) {
        return false;
    }
    
    
    if (!user_exists(uid)) {
        fprintf(stderr, "[USER] User %s does not exist\n", uid);
        return false;
    }
    
    erase_password_file(uid);
    
    erase_login_file(uid);
    
    printf("[USER] Unregistered user: %s (directories preserved)\n", uid);
    return true;
}


bool change_password(const char* uid, const char* old_password, const char* new_password) {
    if (!validate_uid(uid) || !validate_password(old_password) || !validate_password(new_password)) {
        return false;
    }
    
    
    int auth_result = authenticate_user(uid, old_password);
    if (auth_result != 1) {
        return false;
    }
    
    
    if (!write_password_file(uid, new_password)) {
        fprintf(stderr, "[USER] Failed to update password for %s\n", uid);
        return false;
    }
    
    printf("[USER] Password changed for user: %s\n", uid);
    return true;
}
