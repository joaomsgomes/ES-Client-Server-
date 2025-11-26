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

    // Inicializar sistema de utilizadores
    init_user_system();
    printf("ES Server started on UDP port %s\n", MYPORT);

    // TCP SETUP AND SERVER SECTION

    memset(&inputs, 0, sizeof(inputs));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    getaddrinfo(NULL, "58001", &hints, &res);
    tfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    bind(tfd, res->ai_addr, res->ai_addrlen);
    
    if (listen(tfd, 5) == -1 ) {
        sprintf(prt_str,"Bind error TCP server\n");
        write(1,prt_str,strlen(prt_str));
        exit(1);
    }
    
    freeaddrinfo(res);

    FD_ZERO(&inputs); // Clear input mask
    FD_SET(0, &inputs); // Set standard input channel on
    FD_SET(ufd, &inputs); // Set UDP channel on
    FD_SET(tfd, &inputs); // Set TCP channel on

    max_fd = tfd; // REVER --> Pode prejudicar o funcionamento do TCP Channel

    while(1)
    {
        testfds = inputs; // Reload mask
    //        printf("testfds byte: %d\n",((char *)&testfds)[0]); // Debug
        memset((void *)&timeout,0,sizeof(timeout));
        timeout.tv_sec=10;

        out_fds=select(max_fd+1,&testfds,(fd_set *)NULL,(fd_set *)NULL,(struct timeval *) &timeout);
    // testfds is now '1' at the positions that were activated by FD_SET
    //        printf("testfds byte: %d\n",((char *)&testfds)[0]); // Debug

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
                    addrlen = sizeof(_useraddr);
                    ret=recvfrom(ufd,prt_str,80,0,(struct sockaddr *)&_useraddr,&addrlen);
                    if(ret>0)
                    {
                        prt_str[ret]='\0'; // Null terminate
                        printf("---UDP received: %s",prt_str);
                        errcode=getnameinfo( (struct sockaddr *) &_useraddr,addrlen,host,sizeof host, service,sizeof service,0);
                        if(errcode==0)
                            printf("       From [%s:%s]\n",host,service);
                        
                        // Route UDP commands
                        if(strncmp(prt_str, CMD_LOGIN, 3) == 0) {
                            handle_login(ufd, prt_str, &_useraddr, addrlen);
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
                // Input done by TCP socket 

                //REVER ANTES DE CONTINUAR
                
                if(FD_ISSET(tfd,&testfds)) {
                    addrlen = sizeof(_useraddr);
                    ret = accept(tfd, (struct sockaddr *)&_useraddr, &addrlen);
                    if(ret>0) {
                        printf("New TCP connection: fd=%d\n", ret);
                        FD_SET(ret, &inputs);  // Adicionar ao select!
                        if(ret > max_fd) max_fd = ret;
                    }
                }


                break;

        }
    }



}

