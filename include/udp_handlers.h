#ifndef UDP_HANDLERS_H
#define UDP_HANDLERS_H

#include <netinet/in.h>
#include <sys/socket.h>

/**
 * UDP Protocol Handlers
 * Process UDP commands received by the server
 */

// Login handler
void handle_login(int sockfd, char* message, struct sockaddr_in* client_addr, socklen_t addrlen);

// Logout handler
void handle_logout(int sockfd, char* message, struct sockaddr_in* client_addr, socklen_t addrlen);

// Unregister handler
void handle_unregister(int sockfd, char* message, struct sockaddr_in* client_addr, socklen_t addrlen);

// My events handler
void handle_my_events(int sockfd, char* message, struct sockaddr_in* client_addr, socklen_t addrlen);

// My reservations handler
void handle_my_reservations(int sockfd, char* message, struct sockaddr_in* client_addr, socklen_t addrlen);

#endif // UDP_HANDLERS_H
