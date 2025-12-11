#include "utils.h"
#include <string.h>
#include <ctype.h>
#include <time.h>


void show_help() {
    printf("\n╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║                    ES CLIENT - Commands                      ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║  register UID password                                       ║\n");
    printf("║    - Register new user (UDP)                                 ║\n");
    printf("║                                                               ║\n");
    printf("║  unregister UID password                                     ║\n");
    printf("║    - Delete user account (UDP)                               ║\n");
    printf("║                                                               ║\n");
    printf("║  login UID password                                          ║\n");
    printf("║    - Login to server (UDP)                                   ║\n");
    printf("║                                                               ║\n");
    printf("║  logout                                                      ║\n");
    printf("║    - Logout from current session (UDP)                       ║\n");
    printf("║                                                               ║\n");
    printf("║  list                                                        ║\n");
    printf("║    - List all events on server (TCP)                         ║\n");
    printf("║                                                               ║\n");
    printf("║  create name event_fname event_date num_attendees            ║\n");
    printf("║    - Create new event (TCP, requires login)                  ║\n");
    printf("║    - name: max 10 alphanumeric characters                    ║\n");
    printf("║    - event_fname: description file (max 10MB)                ║\n");
    printf("║    - event_date: dd-mm-yyyy format                           ║\n");
    printf("║    - num_attendees: 10-999 seats                             ║\n");
    printf("║    Example: create Concert poster.jpg 31-12-2025 500         ║\n");
    printf("║                                                               ║\n");
    printf("║  pass UID old_password new_password                          ║\n");
    printf("║    - Change password (TCP, requires login)                   ║\n");
    printf("║                                                               ║\n");
    printf("║  reserve EID num_seats                                       ║\n");
    printf("║    - Reserve seats for event (TCP, requires login)           ║\n");
    printf("║                                                               ║\n");
    printf("║  close EID                                                   ║\n");
    printf("║    - Close your event (TCP, requires login)                  ║\n");
    printf("║                                                               ║\n");
    printf("║  myevents (or mye)                                           ║\n");
    printf("║    - List your created events (UDP, requires login)          ║\n");
    printf("║                                                               ║\n");
    printf("║  help                                                         ║\n");
    printf("║    - Show this help message                                  ║\n");
    printf("║                                                               ║\n");
    printf("║  exit                                                         ║\n");
    printf("║    - Exit application                                        ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
}



bool validate_uid(const char* uid) {
    if (!uid || strlen(uid) != 6) return false;
    for (int i = 0; i < 6; i++) {
        if (!isdigit(uid[i])) return false;
    }
    return true;
}

bool validate_password(const char* password) {
    if (!password || strlen(password) != 8) return false;
    for (int i = 0; i < 8; i++) {
        if (!isalnum(password[i])) return false;
    }
    return true;
}

bool validate_event_name(const char* name) {
    if (!name) return false;
    int len = strlen(name);
    if (len == 0 || len > 10) return false;
    for (int i = 0; i < len; i++) {
        if (!isalnum(name[i])) return false;
    }
    return true;
}

bool validate_date(const char* date) {
    if (!date || strlen(date) != 10) return false;
    // Format: dd-mm-yyyy
    if (date[2] != '-' || date[5] != '-') return false;
    // Validate digits
    for (int i = 0; i < 10; i++) {
        if (i == 2 || i == 5) continue;
        if (!isdigit(date[i])) return false;
    }
    // Validate day, month, year ranges
    int day = (date[0] - '0') * 10 + (date[1] - '0');
    int month = (date[3] - '0') * 10 + (date[4] - '0');
    int year = (date[6] - '0') * 1000 + (date[7] - '0') * 100 + 
               (date[8] - '0') * 10 + (date[9] - '0');
    
    if (day < 1 || day > 31) return false;
    if (month < 1 || month > 12) return false;
    if (year < 2025) return false;
    
    return true;
}

void get_current_date(char* buffer) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    sprintf(buffer, "%02d-%02d-%04d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);
}

int compare_dates(const char* date1, const char* date2) {
    // Returns: -1 if date1 < date2, 0 if equal, 1 if date1 > date2
    int y1, m1, d1, y2, m2, d2;
    sscanf(date1, "%d-%d-%d", &d1, &m1, &y1);
    sscanf(date2, "%d-%d-%d", &d2, &m2, &y2);
    
    if (y1 != y2) return (y1 < y2) ? -1 : 1;
    if (m1 != m2) return (m1 < m2) ? -1 : 1;
    if (d1 != d2) return (d1 < d2) ? -1 : 1;
    return 0;
}