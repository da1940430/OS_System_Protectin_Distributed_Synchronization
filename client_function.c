#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>

#include "pre_defined.h"

int create_data_socket(int sd)
{
    int sd_read;
    struct addrinfo hints_read, *info_read;
    memset(&hints_read, 0, sizeof(hints_read));

    hints_read.ai_family = AF_UNSPEC;      //whatever IPv4 or IPv6
    hints_read.ai_socktype = SOCK_STREAM;  //TCP
    hints_read.ai_flags = AI_PASSIVE;      //add my IP automatically

    int port_int = rand() % 50001 + 10000; //port 10000 ~ 60000
    char port[8];
    sprintf(port, "%d", port_int);
    while (getaddrinfo(NULL, port, &hints_read, &info_read) != 0);

    /* create socket and wait for connecting */
    if ((sd_read = socket(info_read->ai_family, info_read->ai_socktype, info_read->ai_protocol)) == -1)
        DIE("create socket failed");
    int bReuseaddr = TRUE;
    if(setsockopt(sd_read, SOL_SOCKET, SO_REUSEADDR, &bReuseaddr, sizeof(bReuseaddr)) == -1)
        DIE("setsockopt failed");
    if (bind(sd_read, info_read->ai_addr, info_read->ai_addrlen) == -1)
        DIE("bind failed");
    if (listen(sd_read, 1) == -1)
        DIE("listen failed");
    if(send(sd, port, sizeof(port), 0) == -1)
        DIE("send port failed");
    return sd_read;
}