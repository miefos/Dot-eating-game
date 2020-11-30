#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 20
#define MIN_CLIENTS 2 // to start the game
#define INITIAL_N_DOTS 6
#define BUFFER_SIZE 1024
#define MAX_PACKET_SIZE 10000
#define INIT_SIZE 10
#define INIT_LIVES 1
#define MAX_X 1000
#define MAX_Y 1000
#define MAX_MESSAGE_SIZE 511
#define TIME_LIM 180 // in sec

typedef struct {
  int socket;
  unsigned char ID;
  unsigned char ready;
  // unsigned char connected; // 0 - this client is not connected, 1 - this client is connected
  unsigned char has_introduced; // meaning the 0th packet is received (at first 0, when receives set it to 1)
  char username[256];
  char color[7];
  unsigned int x;
  unsigned int y;
  unsigned int size;
  unsigned int score;
  unsigned int lives;
  unsigned int FOR_TESTING_ONLY;
} client_struct;

typedef struct {
  unsigned int x;
  unsigned int y;
} dot;

int _create_packet_0(unsigned char *p, char *name, char *color);
int _create_packet_1(unsigned char *p, unsigned char g_id, unsigned char p_id, unsigned int p_init_size, unsigned int x_max, unsigned int y_max, unsigned int time_lim, unsigned int n_lives);
int _create_packet_2(unsigned char *p, unsigned char g_id, unsigned char p_id, unsigned char r_char);
int _create_packet_3(unsigned char *p, unsigned char g_id, client_struct **clients, unsigned short int n_dots, dot **dots, unsigned int time_left);
int _create_packet_4(unsigned char *p, unsigned char *g_id, unsigned char *p_id, char w, char a, char s, char d);
int _create_packet_5(unsigned char* p, unsigned char g_id, unsigned char p_id, unsigned int score, unsigned int time_passed);
int _create_packet_6(unsigned char* p, unsigned char g_id, client_struct** clients, unsigned char curr_player_id, unsigned int curr_player_score);
int _create_packet_7(unsigned char* p, unsigned char g_id, unsigned char p_id, char* message);

int _packet6_helper_process_clients(client_struct** clients, int* n_clients, unsigned char* p_data, int* clients_total_len, unsigned char *xor);

int send_packet_1(client_struct *client);

int process_packet_0(unsigned char *p_dat, client_struct *client);
int process_packet_1(unsigned char *p_dat, int c_socket, int *client_status, unsigned char *g_id, unsigned char *p_id);
int process_packet_7 (unsigned char* p_dat);

void process_incoming_packet(unsigned char *p, int size_header, int size_data, client_struct *client, int c_socket, int *client_status, unsigned char *g_id, unsigned char *p_id, int is_server);
int recv_byte (unsigned char *packet_in, int *packet_cursor, int *current_packet_data_size, int *packet_status, int is_server, client_struct *client, int client_socket, int *client_status, unsigned char *g_id, unsigned char *p_id);
int send_prepared_packet(unsigned char *p, int p_size, int socket);
int get_username_color(char *username, char *color);
int process_str(char *str, unsigned char *xor, unsigned char *p_part);
int process_int_lendian(int n, unsigned char *p_part, unsigned char *xor);
int process_short_int_lendian(short int n, unsigned char *p_part, unsigned char *xor);
int set_packet_header(unsigned char type, unsigned char *p, unsigned int N_LEN, unsigned int npk, unsigned char *xor);
