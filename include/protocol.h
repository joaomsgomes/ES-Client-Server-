#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>


// Códigos de comando UDP
#define CMD_LOGIN "LIN"
#define CMD_LOGOUT "LOU"
#define CMD_UNREGISTER "UNR"
#define CMD_MYEVENTS "LME"
#define CMD_MY_RESERVATIONS "LMR"

// Códigos de resposta UDP
#define RSP_LOGIN "RLI"
#define RSP_LOGOUT "RLO"
#define RSP_UNREGISTER "RUR"
#define RSP_MYEVENTS "RME"
#define RSP_MY_RESERVATIONS "RMR"

// Códigos de comando TCP
#define CMD_CREATE "CRE"
#define CMD_CLOSE "CLS"
#define CMD_LIST "LST"
#define CMD_SHOW "SED"
#define CMD_RESERVE "RID"
#define CMD_CHANGE_PASS "CPS"

// Códigos de resposta TCP
#define RSP_CREATE "RCE"
#define RSP_CLOSE "RCL"
#define RSP_LIST "RLS"
#define RSP_SHOW "RSE"
#define RSP_RESERVE "RRI"
#define RSP_CHANGE_PASS "RCP"

// Status codes
#define STATUS_OK "OK"
#define STATUS_NOK "NOK"
#define STATUS_ERR "ERR"
#define STATUS_REG "REG"
#define STATUS_UNR "UNR"
#define STATUS_NLG "NLG"
#define STATUS_WRP "WRP"
#define STATUS_NOE "NOE"
#define STATUS_EOW "EOW"
#define STATUS_SLD "SLD"
#define STATUS_PST "PST"
#define STATUS_CLO "CLO"
#define STATUS_ACC "ACC"
#define STATUS_REJ "REJ"
#define STATUS_CLS "CLS"
#define STATUS_NID "NID"

// Data Structures for Messages

typedef enum {
    CMD_TYPE_UNKNOWN = 0,
    CMD_TYPE_LOGIN,
    CMD_TYPE_CREATE,
    CMD_TYPE_LOGOUT,
    CMD_TYPE_CLOSE,
    CMD_TYPE_MY_EVENTS,
    CMD_TYPE_LIST,
    CMD_TYPE_RESERVE,
    CMD_TYPE_UNREGISTER,
    CMD_TYPE_CHANGE_PASSWORD,
    CMD_TYPE_SHOW,
    CMD_TYPE_MY_RESERVATIONS,
    CMD_TYPE_HELP,
    CMD_TYPE_EXIT
} CommandType;

#endif