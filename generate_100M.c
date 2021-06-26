#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main(void){
    char path[128] = "../clientFolder/file_100M";
    FILE *fp = fopen(path, "w");
    if(fp == NULL){
        perror("open file failed");
        exit(0);
    }
    int gigabyte = (int)pow(10,8);

    for(int i=0;i<gigabyte;i++)
        fwrite("#", 1, sizeof(char), fp);
    fclose(fp);

    exit(0);
}