#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 20
#define MIN_CLIENTS 2 /* to start the game */
#define MAX_DOTS 40
#define MAX_UNREADY_TIME 1 /* time which is allowed player to get ready... */
#define BUFFER_SIZE 1024
#define MAX_PACKET_SIZE 10000
#define INIT_SIZE 850
#define INIT_LIVES 1
#define MAX_X 800
#define MAX_Y 800
#define CIRCLE_RADIUS 7
#define N_POINTS_FOR_C_RADIUS_UNIT 30
#define GET_PERCENTS_OF_EATEN_PLAYER 40
#define MAX_MESSAGE_SIZE 511
#define SPEED 200.0
#define MIN_SPEED 60.0
#define SIZE_SPEED_COEFFICIENT 0.002
#define BASIC_TEXT_PADDING 5
#define BORDER_SIZE 8
#define TIME_LIM 100 /* in sec */

typedef struct {
  int socket;
  unsigned char ID;
  unsigned char ready;
  unsigned char has_introduced; /* meaning the 0th packet is received (at first 0, when receives set it to 1) */
  char username[256];
  char color[7];
  unsigned char wasd[4];
  float x; /* in packet unsigned int */
  float y; /* in packet unsigned int */
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
  unsigned int active_dots;
  int clients_active; /* Please do use this only in client side.. Not initialized in server */
  /* probably can use client_count in server if really needed but check .. */
} game;

/*
 * this struct is created ONLY to pass params to process_incoming_packet
 * since threads can accept only 1 void* parameter...
 * Later Note: reason not threads anymore since not using for each packet received new thread but still stays the same.
 * */
typedef struct {
    unsigned char *p;
    int size_header;
    int size_data;
    client_struct *client;
    client_struct **clients;
    dot **dots;
    int c_socket;
    int *client_status;
    game *current_game;
    unsigned char *p_id;
    int is_server;
} process_inc_pack_struct;

int _create_packet_0(unsigned char *p, char *name, char *color);
int _create_packet_1(unsigned char *p, unsigned char g_id, unsigned char p_id, unsigned int p_init_size, unsigned int x_max, unsigned int y_max, unsigned int time_lim, unsigned int n_lives);
int _create_packet_2(unsigned char *p, unsigned char g_id, unsigned char p_id, unsigned char r_char);
int _create_packet_3(unsigned char *p, unsigned char g_id, client_struct **clients, unsigned short int n_dots, dot **dots, unsigned int time_left, unsigned int npk);
int _create_packet_4(unsigned char *p, unsigned char *g_id, unsigned char *p_id, char w, char a, char s, char d, unsigned int npk);
int _create_packet_5(unsigned char* p, unsigned char g_id, unsigned char p_id, unsigned int score, unsigned int time_passed);
int _create_packet_6(unsigned char* p, unsigned char g_id, client_struct** clients, unsigned char curr_player_id, unsigned int curr_player_score);
int _create_packet_7(unsigned char* p, unsigned char g_id, unsigned char p_id, char* message);

int _packet6_helper_process_clients(client_struct** clients, int* n_clients, unsigned char* p_data, int* clients_total_len, unsigned char *xor);

int process_packet_0(unsigned char *p_dat, client_struct *client, game *current_game);
int process_packet_1(unsigned char *p_dat, int *client_status, game *current_game, client_struct *client);
int process_packet_2(unsigned char* p_dat, client_struct* client, game *current_game);
int process_packet_3(unsigned char* p, int* client_status, client_struct **clients, game *current_game, dot **dots);
int process_packet_4(unsigned char* p_dat, client_struct* client);
int process_packet_5(unsigned char* p_dat, int *client_status);
int process_packet_6(unsigned char* p_dat, int* client_status);
int process_packet_7 (unsigned char *p_dat);

void* process_incoming_packet(void* process_inc_pack_struct);
int recv_byte (unsigned char *packet_in, int *packet_cursor, int *current_packet_data_size, int *packet_status, int is_server, client_struct *client, int client_socket, int *client_status, unsigned char *p_id, pthread_t *process_packet_thread, game *current_game, client_struct** clients, dot** dots);
int send_prepared_packet(unsigned char *p, int p_size, int socket);
int process_str(char *str, unsigned char *xor, unsigned char *p_part);
int process_int_lendian(int n, unsigned char *p_part, unsigned char *xor);
int process_short_int_lendian(short int n, unsigned char *p_part, unsigned char *xor);
int set_packet_header(unsigned char type, unsigned char *p, unsigned int N_LEN, unsigned int npk, unsigned char *xor);
double getRadius(client_struct *player);
