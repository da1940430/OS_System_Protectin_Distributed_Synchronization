#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "pre_defined.h"
#include "server_function.h"

pthread_rwlock_t lock;

void *command_socket(void *arg)
{
    int sd = *((int *)arg); //get socket descriptor
    char recv_data[MSS];
    char username[STR_LEN], password[STR_LEN], group[STR_LEN];
    while (recv(sd, recv_data, MSS, 0) != 0) //0 means client's socket has closed
    {
        puts("ready recv data!");
        /* login */
        if (strcmp(recv_data, "account checking") == 0){
            user_checking(sd, username, password, group, sizeof(username));
        }

        char recv_temp[STR_LEN], *command;
        strcpy(recv_temp, recv_data);
        command = strtok(recv_temp, " "); //create command
        /* command function */
        if (strcmp(command, "create") == 0){
            create_command(sd, recv_data, username, group);
        } else if (strcmp(command, "chmod") == 0)
        {
            chmod_command(sd, recv_data, username, group);
        }
        else if (strcmp(command, "read") == 0)
        {
            pthread_rwlock_rdlock(&lock);
            puts("read");
            int sd_read = read_command(sd, recv_data, username, group);
            close(sd_read);
            pthread_rwlock_unlock(&lock);
        }
        else if (strcmp(command, "write") == 0)
        {
            pthread_rwlock_wrlock(&lock);
            puts("write");
            int sd_write = write_command(sd, recv_data, username, group);
            close(sd_write);
            pthread_rwlock_unlock(&lock);
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    pthread_rwlock_init(&lock, NULL);
    int sd;

    struct addrinfo hints, *info;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;                            //whatever IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;                        //TCP
    hints.ai_flags = AI_PASSIVE;                            //add my IP automatically
    if (getaddrinfo(NULL, SERVER_PORT, &hints, &info) != 0) //port 8080
        DIE("getaddrinfo failed");

    /* create socket and wait for connecting */
    if ((sd = socket(info->ai_family, info->ai_socktype, info->ai_protocol)) == -1)
        DIE("create socket failed");
    int bReuseaddr = TRUE;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &bReuseaddr, sizeof(bReuseaddr)) == -1)
        DIE("setsockopt failed");
    if (bind(sd, info->ai_addr, info->ai_addrlen) == -1)
        DIE("bind failed");
    if (listen(sd, 10) == -1)
        DIE("listen failed");

    int ad;
    struct sockaddr client;
    socklen_t client_len = sizeof(client);

    /* wait for connecting */
    while ((ad = accept(sd, &client, &client_len)) != -1)
    {
        pthread_t pth;
        pthread_create(&pth, NULL, command_socket, &ad);
    }

    if (close(sd) == -1)
        puts("server close failed");

    return 0;
}