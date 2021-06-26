#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "pre_defined.h"
#include "client_function.h"

void identity_sel(int[]);

int main(int argc, char *argv[])
{
    int sd;

    struct addrinfo hints, *info;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     //whatever IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; //TCP
    if (getaddrinfo(LOCALHOST, SERVER_PORT, &hints, &info) != 0)
        DIE("getaddrinfo failed");

    /* create socket and connect */
    if ((sd = socket(info->ai_family, info->ai_socktype, info->ai_protocol)) == -1)
        DIE("create socket failed");
    if (connect(sd, info->ai_addr, info->ai_addrlen) == -1)
        DIE("connect failed");

    char send_data[MSS];
    char recv_data[MSS];
    /* login */
    while (1)
    {
        /* get account id */
        puts("Please input your account ID");
        char account[STR_LEN];
        fgets(account, sizeof(account), stdin);
        strtok(account, "\n");

        /* get password */
        puts("Please input your account password");
        char password[STR_LEN];
        fgets(password, sizeof(password), stdin);
        strtok(password, "\n");

        /* check account and password */
        strcpy(send_data, "account checking");
        if (send(sd, send_data, sizeof(send_data), 0) == -1) //send request
            perror("send account checking msg failed");
        if (recv(sd, recv_data, sizeof(recv_data), 0) == -1) //recv reply
            perror("recv data failed");
        if (strcmp(recv_data, "checking ready") == 0)
        {
            //send account ID and password
            if (send(sd, account, sizeof(account), 0) == -1)
                perror("send account data failed");
            if (send(sd, password, sizeof(password), 0) == -1)
                perror("send password data failed");
            if (recv(sd, recv_data, sizeof(recv_data), 0) == -1)
                perror("recv allowed failed");
            if (strcmp(recv_data, "allowed") == 0)
                break;
            else if (strcmp(recv_data, "refused") == 0)
                puts("connect refused");
        }
    }
    /* send command */
    while (1)
    {
        printf("command: ");
        fgets(send_data, sizeof(send_data), stdin);
        strtok(send_data, "\n");
        char command[STR_LEN];
        strcpy(command, send_data);
        if (strcmp(send_data, "exit") == 0) //quit client
            break;
        if (send(sd, send_data, sizeof(send_data), 0) == -1)
            perror("send command failed");
        if (recv(sd, recv_data, sizeof(recv_data), 0) == -1)
            perror("recv command reply failed");

        if (strcmp(recv_data, "File exist!") == 0)
        {
            puts(recv_data);
            continue;
        }
        else if (strcmp(recv_data, "chmod refused!") == 0)
        {
            puts(recv_data);
            continue;
        }
        else if (strcmp(recv_data, "read refused!") == 0 || 
                strcmp(recv_data, "File doesn't exist!") == 0||
                strcmp(recv_data, "write refused!") == 0)
        {
            puts(recv_data);
            continue;
        }
        else if (strcmp(recv_data, "read allowed!") == 0)
        {
            int sd_read = create_data_socket(sd), ad;
            if(sd_read == -1){
                continue;
            }
            struct sockaddr_in server;
            socklen_t server_len;
            server_len = sizeof(server);
            ad = accept(sd_read, &server, &server_len);
            if(ad == -1){
                perror("accept read failed");
                continue;
            }
            strtok(command, " ");
            char *file_name = strtok(NULL, " \0");
            char file_path[STR_LEN] = CLIENT_FILE_PATH;
            strcat(file_path, file_name);
            FILE *fp = fopen(file_path, "w+");
            if(fp == NULL){
                perror("file open failed");
                continue;
            }
            while(recv(ad, recv_data, sizeof(recv_data), 0) != 0){
                if(strcmp(recv_data, "Finished!") == 0){
                    puts("read Finished!");
                    break;
                }
                fwrite(recv_data, 1, strlen(recv_data), fp);
            }
            fclose(fp);
        }
        else if (strcmp(recv_data, "write allowed!") == 0)
        {
            int sd_write = create_data_socket(sd), ad;
            if(sd_write == -1)
                continue;
            struct sockaddr_in server;
            socklen_t server_len;
            server_len = sizeof(server);
            ad = accept(sd_write, &server, &server_len);
            if(ad == -1){
                perror("accept write failed");
                continue;
            }
            strtok(command, " ");
            char *file_name = strtok(NULL, " ");
            // char *o_a = strtok(NULL, "\0");
            char file_path[STR_LEN] = CLIENT_FILE_PATH;
            strcat(file_path, file_name);
            FILE *fp = fopen(file_path, "r");
            if(fp == NULL){
                perror("file doesn't exist");
                continue;
            }
            while(fgets(send_data, sizeof(send_data), fp) != NULL){
                if(send(ad, send_data, sizeof(send_data), 0) == -1){
                    perror("send data failed");
                    break;
                }
            }
            strcpy(send_data, "writting Finished!");
            if(send(ad, send_data, sizeof(send_data), 0) == -1)
                perror("send data failed");
            puts("File writting finished");
            fclose(fp);
        }
    }

    if (close(sd) == 0)
        puts("client close success");
    else
        puts("client close failed");

    return 0;
}