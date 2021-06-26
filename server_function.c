#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>

#include "pre_defined.h"
#include "server_function.h"

void user_checking(int sd, char *account, char *password, char *grp, int size)
{
    char send_data[128], recv_data[128];
    strcpy(send_data, "checking ready");
    if (send(sd, send_data, sizeof(send_data), 0) == -1) //reply client ready to recv account data
        perror("send failed");

    /* recv account and password */
    if (recv(sd, account, size, 0) == -1)
        perror("recv account data failed");
    if (recv(sd, password, size, 0) == -1)
        perror("recv password data failed");
    /* account checking */
    FILE *fp = fopen(ACCOUNT_PATH, "r");
    short allow_login = FALSE;
    while (fgets(recv_data, sizeof(recv_data), fp) != NULL)
    {
        char *act_tok, *pwd_tok, *grp_tok;
        act_tok = strtok(recv_data, ":");
        pwd_tok = strtok(NULL, ":");
        grp_tok = strtok(NULL, ":");
        if (strcmp(act_tok, account) == 0 && strcmp(pwd_tok, password) == 0)
        {
            allow_login = TRUE;
            strcpy(send_data, "allowed");
            strcpy(grp, grp_tok); //copy login group name in grp
            if (send(sd, send_data, sizeof(send_data), 0) == -1)
                perror("send allowed failed");
            break;
        }
    }
    if (!allow_login)
    {
        strcpy(send_data, "refused");
        if (send(sd, send_data, sizeof(send_data), 0) == -1)
            perror("send refused failed");
    }
}

int create_command(int sd, char *recv, char *account, char *grp)
{
    char *file_name, *right, send_data[128], origin_command[128];
    strcpy(origin_command, recv);
    strtok(origin_command, " ");
    file_name = strtok(NULL, " ");
    right = strtok(NULL, "\0");

    /* check file exist */
    char path[128];
    strcpy(path, FILE_PATH);
    strcat(path, file_name);
    if (access(path, F_OK) == 0)
    {
        //access() to check file exist
        strcpy(send_data, "File exist!");
        if (send(sd, send_data, sizeof(send_data), 0) == -1)
            perror("send File exist failed");
        return 0;
    }
    else
    {
        strcpy(send_data, "");
        if (send(sd, send_data, sizeof(send_data), 0) == -1)
            perror("send File exist failed");
    }

    /* search capability list */
    FILE *fp = fopen(CAPABILITY_PATH, "r"); //open capability
    FILE *fp_write_in = fopen(CAPABILITY_V2_PATH, "w");
    char capa_data[128];
    while (fgets(capa_data, sizeof(capa_data), fp) != NULL)
    {
        char data_temp[128], *username, *user_group;
        strcpy(data_temp, capa_data);
        username = strtok(data_temp, ":");
        user_group = strtok(NULL, ":");
        if (strcmp(username, account) == 0)
        {
            char write_in[128];
            file_write_in(write_in, capa_data, file_name, right);
            fwrite(write_in, 1, strlen(write_in), fp_write_in);
        }
        else if (strcmp(grp, user_group) == 0 && strcmp(username, account) != 0 && (right[2] == 'r' || right[3] == 'w'))
        {
            char grp_right[STR_LEN];
            strcpy(grp_right, right);
            /* record group right */
            grp_right[0] = '-';
            grp_right[1] = '-';
            grp_right[4] = '-';
            grp_right[5] = '-';
            char write_in[128];
            file_write_in(write_in, capa_data, file_name, grp_right);
            fwrite(write_in, 1, strlen(write_in), fp_write_in);
        }
        else
        {
            char others_right[STR_LEN];
            strcpy(others_right, right);
            /* record others right */
            others_right[0] = '-';
            others_right[1] = '-';
            others_right[2] = '-';
            others_right[3] = '-';
            char write_in[128];
            file_write_in(write_in, capa_data, file_name, others_right);
            fwrite(write_in, 1, strlen(write_in), fp_write_in);
        }
    }
    fclose(fp);
    fclose(fp_write_in);

    /* replace old capability list */
    unlink(CAPABILITY_PATH);
    rename(CAPABILITY_V2_PATH, CAPABILITY_PATH);

    /* create new file */
    strcpy(path, FILE_PATH);
    strcat(path, file_name);
    creat(path, S_IRWXU);

    return 0;
}

int check_right(char *account, char *file_name, char *mode)
{
    FILE *fp = fopen(CAPABILITY_PATH, "r");
    char capa_data[STR_LEN], item[STR_LEN] = "";
    file_item(item, file_name);
    while (fgets(capa_data, sizeof(capa_data), fp) != NULL)
    {
        char temp[STR_LEN];
        strcpy(temp, capa_data);
        char *username = strtok(temp, ":");
        if (strcmp(account, username) == 0)
        {
            char *search_ptr = strstr(capa_data, item);
            search_ptr = strstr(search_ptr, "|");

            /* check owner right */
            if (strcmp(mode, "owner") == 0)
            {
                if (search_ptr[1] == 'r' || search_ptr[2] == 'w')
                    return TRUE;
                else
                    return FALSE;
            }
            else if (strcmp(mode, "read") == 0)
            {
                if (search_ptr[1] == 'r' || search_ptr[3] == 'r' || search_ptr[5] == 'r')
                    return TRUE;
                else
                    return FALSE;
            }
            else if (strcmp(mode, "write") == 0)
            {
                if (search_ptr[2] == 'w' || search_ptr[4] == 'w' || search_ptr[6] == 'w')
                    return TRUE;
                else
                    return FALSE;
            }
        }
    }
    return FALSE;
}

int check_file(char *file_name)
{
    char path[STR_LEN] = FILE_PATH;
    strcat(path, file_name);
    int check = access(path, F_OK);
    if (!check)
        return TRUE;
    else
        return FALSE;
}

int chmod_command(int sd, char *recv, char *account, char *grp)
{
    char recv_data[STR_LEN];
    strcpy(recv_data, recv);
    strtok(recv_data, " "); //command
    char *file_name, *right;
    file_name = strtok(NULL, " ");
    right = strtok(NULL, " ");
    /* check file exist */
    if (!check_file(file_name))
    {
        char send_data[MSS] = "File doesn't exist!";
        if (send(sd, send_data, sizeof(send_data), 0) == -1)
            perror("send File exist failed");
        return 0;
    }
    /* check right */
    if (!check_right(account, file_name, "owner"))
    {
        char send_data[MSS] = "chmod refused!";
        if (send(sd, send_data, sizeof(send_data), 0) == -1)
            perror("send File exist failed");
        return 0;
    }

    /* search capability list */
    FILE *fp = fopen(CAPABILITY_PATH, "r+");
    char capa_data[STR_LEN];
    int seek_index = 0;
    while (fgets(capa_data, sizeof(capa_data), fp) != NULL)
    {
        char data_temp[STR_LEN], *username, *group;
        strcpy(data_temp, capa_data);
        username = strtok(data_temp, ":");
        group = strtok(NULL, ":");

        char item[STR_LEN] = " ";
        file_item(item, file_name);
        char data_temp2[STR_LEN];
        strcpy(data_temp2, capa_data);
        char *search_ptr = strstr(data_temp2, item); //get file position
        strtok(search_ptr, "|");
        int data_len = strlen(data_temp2);
        if (strcmp(account, username) == 0) //owner right
        {
            fseek(fp, seek_index + data_len + 1, SEEK_SET);
            for (int i = 0; i < strlen(right); i++)
                fwrite(&right[i], 1, 1, fp);
        }
        else if (strcmp(group, grp) == 0)
        { //grp right
            fseek(fp, seek_index + data_len + 3, SEEK_SET);
            for (int i = 2; i < strlen(right) - 2; i++)
                fwrite(&right[i], 1, 1, fp);
        }
        else
        { //others right
            fseek(fp, seek_index + data_len + 5, SEEK_SET);
            for (int i = 4; i < strlen(right); i++)
                fwrite(&right[i], 1, 1, fp);
        }
        seek_index += strlen(capa_data);
        fseek(fp, seek_index, SEEK_SET);
    }
    fclose(fp);

    char send_data[MSS] = "";
    if (send(sd, send_data, sizeof(send_data), 0) == -1)
        perror("send File exist failed");

    return 0;
}

int read_command(int sd, char *origin, char *account, char *grp)
{
    char command[STR_LEN];
    strcpy(command, origin);
    strtok(command, " ");
    char *file_name = strtok(NULL, "\0");
    /* check file exist */
    if (!check_file(file_name))
    {
        char send_data[MSS] = "File doesn't exist!";
        if (send(sd, send_data, sizeof(send_data), 0) == -1)
            perror("send File exist failed");
        return 0;
    }
    if (!check_right(account, file_name, "read"))
    {
        char send_data[MSS] = "read refused!";
        if (send(sd, send_data, sizeof(send_data), 0) == -1)
            DIE("send \"read refused!\" failed");
        return 0;
    }
    /* create socket */
    struct addrinfo hints, *res;
    int sockfd;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    char recv_data[MSS];
    char send_data[MSS] = "read allowed!";
    if (send(sd, send_data, sizeof(send_data), 0) == -1) //send allow reading msg
        DIE("send \"read allowed\" failed");
    if (recv(sd, recv_data, sizeof(recv_data), 0) == -1) //get port
        DIE("recv port failed");
    if (getaddrinfo(LOCALHOST, recv_data, &hints, &res) != 0)
        DIE("getaddrinfo failed");
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1)
        DIE("create socket failed");
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1)
        DIE("connect failed");

    /* read file and transmit */
    char path[STR_LEN] = FILE_PATH;
    strcat(path, file_name);
    FILE *fp = fopen(path, "r");
    while (fgets(send_data, sizeof(send_data) - 1, fp) != NULL)
    {
        if (send(sockfd, send_data, sizeof(send_data), 0) == -1)
            DIE("send data failed");
    }
    strcpy(send_data, "Finished!");
    if (send(sockfd, send_data, sizeof(send_data), 0) == -1)
        DIE("send data failed");

    fclose(fp);
    return 0;
}

int write_command(int sd, char *origin, char *account, char *group)
{
    char command[STR_LEN];
    strcpy(command, origin);
    strtok(command, " ");
    char *file_name = strtok(NULL, " ");
    char *o_a = strtok(NULL, "\0");
    /* check right */
    if (!check_right(account, file_name, "write"))
    {
        char send_data[MSS] = "write refused!";
        if (send(sd, send_data, sizeof(send_data), 0) == -1)
            DIE("send \"write refused!\" failed");
        return 0;
    }

    /* create socket */
    struct addrinfo hints, *res;
    int sockfd;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    char recv_data[MSS];
    char send_data[MSS] = "write allowed!";
    if (send(sd, send_data, sizeof(send_data), 0) == -1) //send allow reading msg
        DIE("send \"write allowed\" failed");
    if (recv(sd, recv_data, sizeof(recv_data), 0) == -1) //get port
        DIE("recv port failed");
    if (getaddrinfo(LOCALHOST, recv_data, &hints, &res) != 0)
        DIE("getaddrinfo failed");
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1)
        DIE("create socket failed");
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1)
        DIE("connect failed");

    /* read file and transmit */
    char path[STR_LEN] = FILE_PATH;
    strcat(path, file_name);
    FILE *fp;
    if (strcmp(o_a, "o") == 0)
        fp = fopen(path, "w");
    else if (strcmp(o_a, "a") == 0)
        fp = fopen(path, "a");

    while (recv(sockfd, recv_data, sizeof(recv_data), 0) != 0)
    {
        if (strcmp(recv_data, "writting Finished!") == 0)
            break;
        fwrite(recv_data, 1, strlen(recv_data), fp);
    }

    fclose(fp);
    return 0;
}

void file_item(char *item, char *file_name)
{
    strcat(item, file_name);
    strcat(item, "|");
}

void file_write_in(char *write_in, char *recv, char *file_name, char *right)
{
    strcpy(write_in, recv);
    strtok(write_in, "\n");
    strcat(write_in, " ");
    strcat(write_in, file_name);
    strcat(write_in, "|");
    strcat(write_in, right);
    strcat(write_in, "\n");
}