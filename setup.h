#include <sys/socket.h>
#include <arpa/inet.h>

int client_setup(int argc, char **argv, int *port, char *ip);
int server_network_setup(int* main_socket, struct sockaddr_in* server_address, int port);
int server_parse_args(int argc, char **argv, int *port);
int get_named_argument(char* key, int argc, char **argv, char** result);
int get_port(char* key, int argc, char** argv);
