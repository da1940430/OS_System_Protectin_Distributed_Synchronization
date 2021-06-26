#include <pthread.h>

void user_checking(int, char *, char *, char *, int);
int create_command(int, char *, char *, char *);
int check_right(char *, char *, char *);
int check_file(char *);
int chmod_command(int, char *, char *, char *);
int read_command(int, char *, char *, char *);
int write_command(int, char *, char *, char *);
void file_item(char *, char *);
void file_write_in(char *, char *, char *, char *);