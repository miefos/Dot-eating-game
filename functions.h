#ifndef MY_FUNCTIONS_H
#define MY_FUNCTIONS_H

int get_named_argument(char* key, int argc, char **argv, char** result);
int get_port(char* key, int argc, char** argv);
int err_mess_return(char *message);

int server_setup(int argc, char **argv, int *port);


#endif
