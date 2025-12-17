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

bool validate_datetime_format(const char* date, const char* time) {
    // Validar formato completo: dd-mm-yyyy hh:mm
    if (!date || strlen(date) != 10) return false;
    if (!time || strlen(time) != 5) return false;
    
    // Validar formato da data: dd-mm-yyyy
    if (date[2] != '-' || date[5] != '-') return false;
    for (int i = 0; i < 10; i++) {
        if (i == 2 || i == 5) continue;
        if (!isdigit(date[i])) return false;
    }
    
    // Validar formato do tempo: hh:mm
    if (time[2] != ':') return false;
    for (int i = 0; i < 5; i++) {
        if (i == 2) continue;
        if (!isdigit(time[i])) return false;
    }
    
    return true;
}

bool validate_datetime_range(const char* date, const char* time) {
    // Validar ranges de data e hora
    int day = (date[0] - '0') * 10 + (date[1] - '0');
    int month = (date[3] - '0') * 10 + (date[4] - '0');
    int year = (date[6] - '0') * 1000 + (date[7] - '0') * 100 + 
               (date[8] - '0') * 10 + (date[9] - '0');
    
    int hour = (time[0] - '0') * 10 + (time[1] - '0');
    int min = (time[3] - '0') * 10 + (time[4] - '0');
    
    // Validar ranges básicos
    if (month < 1 || month > 12) return false;
    if (hour < 0 || hour > 23) return false;
    if (min < 0 || min > 59) return false;
    
    // Validar dia conforme o mês
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // Verificar ano bissexto
    bool is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    if (is_leap && month == 2) {
        days_in_month[1] = 29;
    }
    
    if (day < 1 || day > days_in_month[month - 1]) return false;
    
    return true;
}

bool validate_datetime(const char* date, const char* time) {
    return validate_datetime_format(date, time) && validate_datetime_range(date, time);
}

void get_current_datetime(char* buffer) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    sprintf(buffer, "%02d-%02d-%04d %02d:%02d", 
            t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
            t->tm_hour, t->tm_min);

}

int compare_dates(const char* date1, const char* date2) {
    // Returns: -1 if date1 < date2, 0 if equal, 1 if date1 > date2
    // Suporta: "dd-mm-yyyy" ou "dd-mm-yyyy hh:mm"
    int y1, m1, d1, h1 = 0, min1 = 0;
    int y2, m2, d2, h2 = 0, min2 = 0;
    
    sscanf(date1, "%d-%d-%d %d:%d", &d1, &m1, &y1, &h1, &min1);
    sscanf(date2, "%d-%d-%d %d:%d", &d2, &m2, &y2, &h2, &min2);
    
    if (y1 != y2) return (y1 < y2) ? -1 : 1;
    if (m1 != m2) return (m1 < m2) ? -1 : 1;
    if (d1 != d2) return (d1 < d2) ? -1 : 1;
    if (h1 != h2) return (h1 < h2) ? -1 : 1;
    if (min1 != min2) return (min1 < min2) ? -1 : 1;
    return 0;
}

bool is_date_before_now(const char* date) {
    // Verifica se a data fornecida é anterior à data/hora atual
    // Suporta: "dd-mm-yyyy" ou "dd-mm-yyyy hh:mm"
    char current_datetime[20];
    get_current_datetime(current_datetime);
    return compare_dates(date, current_datetime) < 0;
}