#include <sys/types.h>
#include <sys/socket.h>

#define PORT "58001"
#define BUFFER_SIZE 512
#define AI_PASSIVE 0x0001 // TO DO: Define AI_PASSIVE if not defined

struct addrinfo {
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    socklen_t ai_addrlen;
    struct sockaddr *ai_addr;
    char *ai_canonname;
    struct addrinfo *ai_next;
};
