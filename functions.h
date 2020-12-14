#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 20
#define MIN_CLIENTS 2 /* to start the game */
#define INITIAL_N_DOTS 6
#define MAX_UNREADY_TIME 3 /* after 10 secs set player to ready. Start counting from when has introduced. */
#define BUFFER_SIZE 1024
#define MAX_PACKET_SIZE 10000
#define INIT_SIZE 1000
#define INIT_LIVES 1
#define MAX_X 700
#define MAX_Y 700
#define MAX_MESSAGE_SIZE 511
#define SPEED 1
#define TIME_LIM 180 /* in sec */

typedef struct {
  int socket;
  unsigned char ID;
  unsigned char ready;
  /* unsigned char connected; */ /* 0 - this client is not connected, 1 - this client is connected */
  unsigned char has_introduced; /* meaning the 0th packet is received (at first 0, when receives set it to 1) */
  char username[256];
  char color[7];
  unsigned int x;
  unsigned int y;
  unsigned int size;
  unsigned int score;
  unsigned int lives;
  float connected_time;
} client_struct;

typedef struct {
  unsigned int x;
  unsigned int y;
} dot;

typedef struct {
  unsigned char g_id;
  unsigned int time_left;
  unsigned int time_limit;
  unsigned int initial_size;
  unsigned int max_y, max_x;
  unsigned int max_lives;
  int clients_active;
} game;

/*
 * this struct is created ONLY to pass params to process_incoming_packet
 * since threads can accept only 1 void* parameter...
 * */
typedef struct {
    unsigned char *p;
    int size_header;
    int size_data;
    client_struct *client;
    client_struct **clients;
    int c_socket;
    int *client_status;
    game *current_game;
    unsigned char *p_id;
    int is_server;
} process_inc_pack_struct;

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

int process_packet_0(unsigned char *p_dat, client_struct *client, game *current_game);
int process_packet_1(unsigned char *p_dat, int *client_status, game *current_game, client_struct *client);
int process_packet_7 (unsigned char *p_dat);

void* process_incoming_packet(void* process_inc_pack_struct);
int recv_byte (unsigned char *packet_in, int *packet_cursor, int *current_packet_data_size, int *packet_status, int is_server, client_struct *client, int client_socket, int *client_status, unsigned char *p_id, pthread_t *process_packet_thread, game *current_game, client_struct** clients);
int send_prepared_packet(unsigned char *p, int p_size, int socket);
int get_username_color(char *username, char *color);
int process_str(char *str, unsigned char *xor, unsigned char *p_part);
int process_int_lendian(int n, unsigned char *p_part, unsigned char *xor);
int process_short_int_lendian(short int n, unsigned char *p_part, unsigned char *xor);
int set_packet_header(unsigned char type, unsigned char *p, unsigned int N_LEN, unsigned int npk, unsigned char *xor);
