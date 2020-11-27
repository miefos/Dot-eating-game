#ifndef MY_FUNCTIONS_H
#define MY_FUNCTIONS_H
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define MAX_PACKET_SIZE 10000

int get_named_argument(char* key, int argc, char **argv, char** result);
int get_port(char* key, int argc, char** argv);
int contains_only_hex_digits(char* str);


int get_int_from_4bytes_lendian(unsigned char* the4bytes);
int server_network_setup(int* main_socket, struct sockaddr_in* server_address, int port);
int server_parse_args(int argc, char **argv, int *port);
int client_setup(int argc, char **argv, int *port, char *ip);
int get_username_color(char* username, char* color);
void remove_newline(char *str);
int is_little_endian_system();
int escape_assign(int num, unsigned char* packet);
// int memcpy_and_escape(void* destination, void* source, int size); // returns how many were replaced
// void set_leave_flag(int *flag);

char printable_char(char c);
void print_bytes(void* packet, int count);
int assign_int_to_bytes_lendian_escape(unsigned char* packet_part, int n, int should_escape);
unsigned char get_checksum(unsigned char* packet_header, int length_header_excl_div, unsigned char* packet_data, int data_length);
int create_packet(unsigned char* packet, int type, char* data);



#endif

