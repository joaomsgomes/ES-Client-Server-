#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include "../../include/es_server.h"
#include "../../include/protocol.h"
#include "../../include/user_management.h"
#include "../../include/udp_handlers.h"
#include "../../include/tcp_handlers.h"
#include "../../include/file_system.h"

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

int main(void) {

    char in_str[ARRAY_SIZE];

    fd_set inputs, testfds;
    struct timeval timeout;


    int out_fds,errcode, ret;

    int max_fd;

    char prt_str[90];


    // socket variables
    struct addrinfo hints, *res;
    struct sockaddr_in _useraddr;
    socklen_t addrlen;
    int ufd, tfd; //UDP fd and TCP fd

    char host[NI_MAXHOST], service[NI_MAXSERV];


    // UDP SETUP AND SERVER SECTION
    memset(&hints,0,sizeof(hints));
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_flags=AI_PASSIVE|AI_NUMERICSERV;

    if((errcode=getaddrinfo(NULL,MYPORT,&hints,&res))!=0)
        exit(1);// On error

    ufd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    if(ufd==-1)
        exit(1);

    if(bind(ufd, res->ai_addr, res->ai_addrlen)==-1)
    {
        sprintf(prt_str,"Bind error UDP server\n");
        write(1,prt_str,strlen(prt_str));
        exit(1);// On error
    }
    if(res!=NULL)
        freeaddrinfo(res);

    // Inicializar sistema de ficheiros, utilizadores e eventos
    init_file_system();
    printf("ES Server started on UDP port %s\n", MYPORT);

    // TCP SETUP AND SERVER SECTION (mesma porta que UDP)

    memset(&inputs, 0, sizeof(inputs));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    if((errcode = getaddrinfo(NULL, MYPORT, &hints, &res)) != 0) {
        sprintf(prt_str,"getaddrinfo error TCP server: %s\n", gai_strerror(errcode));
        write(1, prt_str, strlen(prt_str));
        exit(1);
    }
    
    tfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(tfd == -1) {
        sprintf(prt_str,"Socket error TCP server\n");
        write(1, prt_str, strlen(prt_str));
        freeaddrinfo(res);
        exit(1);
    }
    
    if(bind(tfd, res->ai_addr, res->ai_addrlen) == -1) {
        sprintf(prt_str,"Bind error TCP server\n");
        write(1, prt_str, strlen(prt_str));
        freeaddrinfo(res);
        exit(1);
    }
    
    if (listen(tfd, 5) == -1 ) {
        sprintf(prt_str,"Listen error TCP server\n");
        write(1,prt_str,strlen(prt_str));
        freeaddrinfo(res);
        exit(1);
    }
    
    freeaddrinfo(res);
    
    printf("ES Server: TCP listening on port %s\n", MYPORT);

    FD_ZERO(&inputs); // Clear input mask
    FD_SET(0, &inputs); // Set standard input channel on
    FD_SET(ufd, &inputs); // Set UDP channel on
    FD_SET(tfd, &inputs); // Set TCP channel on

    max_fd = tfd; // REVER --> Pode prejudicar o funcionamento do TCP Channel

    while(1)
    {
        testfds = inputs;
    //        printf("testfds byte: %d\n",((char *)&testfds)[0]); // Debug
        memset((void *)&timeout,0,sizeof(timeout));
        timeout.tv_sec=10;

        out_fds=select(max_fd+1,&testfds,(fd_set *)NULL,(fd_set *)NULL,(struct timeval *) &timeout);
        // testfds is now '1' at the positions that were activated by FD_SET

        switch(out_fds)
        {
            case 0:
                printf("\n ---------------Timeout event-----------------\n");
                break;
            case -1:
                perror("select");
                exit(1);
            default:

                // Input done by stdin
                if(FD_ISSET(0,&testfds)) {
                    fgets(in_str,50,stdin);
                    printf("---Input at keyboard: %s\n",in_str);
                    if(!memcmp(in_str,"_STOP",5))
                    {
                        write(1,"Terminating\n",12);
                        exit(0);
                    }
                }
                // Input done by UDP socket
                if(FD_ISSET(ufd,&testfds)) {
                    // Limpar e reinicializar endereço antes de cada recvfrom
                    memset(&_useraddr, 0, sizeof(_useraddr));
                    addrlen = sizeof(_useraddr);
                    ret = recvfrom(ufd,prt_str,80,0,(struct sockaddr *)&_useraddr,&addrlen);
                    if(ret>0)
                    {
                        prt_str[ret]='\0';
                        printf("---UDP received: %s",prt_str);
                        errcode=getnameinfo( (struct sockaddr *) &_useraddr,addrlen,host,sizeof host, service,sizeof service,0);
                        if(errcode==0) {
                            printf("       From [%s:%s], addrlen=%d, family=%d\n",host,service, addrlen, _useraddr.sin_family);
                        } else {
                            printf("       getnameinfo failed: %s\n", gai_strerror(errcode));
                        }
                        
                       
                        if(strncmp(prt_str, CMD_LOGIN, 3) == 0) {
                            handle_login(ufd, prt_str, &_useraddr, addrlen);
                        }
                        else if(strncmp(prt_str, CMD_LOGOUT, 3) == 0) {
                            handle_logout(ufd, prt_str, &_useraddr, addrlen);
                        }
                        else if(strncmp(prt_str, CMD_UNREGISTER, 3) == 0) {
                            handle_unregister(ufd, prt_str, &_useraddr, addrlen);
                        }
                        else if(strncmp(prt_str, CMD_MYEVENTS, 3) == 0) {
                            handle_my_events(ufd, prt_str, &_useraddr, addrlen);
                        }
                        else if(strncmp(prt_str, CMD_MY_RESERVATIONS, 3) == 0) {
                            handle_my_reservations(ufd, prt_str, &_useraddr, addrlen);
                        }
                        else if(!memcmp(prt_str,"_STOP",5))
                        {
                            write(1,"Terminating\n",12);
                            exit(0);
                        }
                        else {
                            printf("       Unknown UDP command\n");
                        }
                    }
                }
                // Input done by TCP socket (listening socket)
                if(FD_ISSET(tfd,&testfds)) {
                    addrlen = sizeof(_useraddr);
                    int client_fd = accept(tfd, (struct sockaddr *)&_useraddr, &addrlen);
                    if(client_fd > 0) {
                        errcode=getnameinfo((struct sockaddr *) &_useraddr, addrlen, 
                                          host, sizeof(host), service, sizeof(service), 0);
                        if(errcode==0)
                            printf("[TCP] New connection from [%s:%s] fd=%d\n", host, service, client_fd);
                        
                        // Ler comando TCP (buffer grande para ficheiros)
                        char *tcp_buffer = malloc(MAX_FILE_SIZE + 1024); 
                        if (!tcp_buffer) {
                            printf("[TCP] ERROR: Memory allocation failed\n");
                            close(client_fd);
                        } else {
                            // Ler dados em loop até receber tudo (pode chegar em múltiplos pacotes)
                            ssize_t total_read = 0;
                            ssize_t bytes_read;
                            
                            bytes_read = read(client_fd, tcp_buffer, MAX_FILE_SIZE + 1024);
                            if (bytes_read > 0) {
                                total_read = bytes_read;
                                // REVER!!!!
                                // Se for CREATE, precisamos ler mais dados (filedata pode vir em pacotes separados)
                                if (bytes_read >= 3 && strncmp(tcp_buffer, CMD_CREATE, 3) == 0) {
                                    struct timeval tv;
                                    tv.tv_sec = 1;  // timeout de 1 segundo
                                    tv.tv_usec = 0;
                                    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
                                    
                                    while (total_read < MAX_FILE_SIZE + 1024) {
                                        bytes_read = read(client_fd, tcp_buffer + total_read, 
                                                        MAX_FILE_SIZE + 1024 - total_read);
                                        if (bytes_read <= 0) break;  // timeout ou fim de dados
                                        total_read += bytes_read;
                                    }
                                }
                                
                                tcp_buffer[total_read] = '\0'; 
                                printf("[TCP] Received %zd bytes (total)\n", total_read);
                                bytes_read = total_read;
                                
                                // Identificar comando (primeiros 3 caracteres)
                                if (bytes_read >= 3) {

                                    if (strncmp(tcp_buffer, CMD_CHANGE_PASS, 3) == 0) {
                                        handle_change_password(client_fd, tcp_buffer, bytes_read);

                                    } else if (strncmp(tcp_buffer, CMD_CREATE, 3) == 0) {
                                        handle_create_event(client_fd, tcp_buffer, bytes_read);
                                    }
                                    else if (strncmp(tcp_buffer, CMD_CLOSE, 3) == 0) {
                                        handle_close_event(client_fd, tcp_buffer, bytes_read);
                                    }
                                    else if (strncmp(tcp_buffer, CMD_LIST, 3) == 0) {
                                        handle_list_events(client_fd, tcp_buffer, bytes_read);  
                                    }
                                    else if (strncmp(tcp_buffer, CMD_SHOW, 3) == 0) {
                                        handle_show_event(client_fd, tcp_buffer, bytes_read);
                                    }
                                    else if(strncmp(tcp_buffer, CMD_RESERVE, 3) == 0) {
                                        handle_reserve_seats(client_fd, tcp_buffer, bytes_read);
                                    }
                                    else {
                                        printf("[TCP] Unknown command\n");
                                    }
                                } else {
                                    printf("[TCP] Invalid command (too short)\n");
                                }
                            } else if (bytes_read == 0) {
                                printf("[TCP] Client closed connection\n");
                            } else {
                                perror("[TCP] recv error");
                            }
                            
                            free(tcp_buffer);
                        }
                        
                        // Fechar conexão (protocolo request-response simples)
                        close(client_fd);
                        printf("[TCP] Connection closed: fd=%d\n", client_fd);
                    }
                }


                break;

        }
    }



}

