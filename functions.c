#include "functions.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h> // toupper, isprint
#include <inttypes.h>
#include <unistd.h>
#include "util_functions.h"
#include "setup.h"

int _create_packet_0(unsigned char* p, char* name, char* color) {
  unsigned char type = 0;
  unsigned int npk = 0;
  unsigned char xor = 0;
  unsigned char name_l = (unsigned char) strlen(name);
  unsigned char N_LEN = name_l + 1 + 6; // 1 - name_l, 6 - color
  int esc = 0; // escape count

  // p header
  int h_size = 11;
  esc += set_packet_header(type, p, N_LEN, npk, &xor);

  // p data
  xor ^= name_l;
  esc += escape_assign(name_l, &p[h_size + esc]); // name_l
  esc += process_str(name, &xor, &p[h_size + 1 + esc]); // name
  esc += process_str(color, &xor, &p[h_size + 1 + name_l + esc]); // color

  // p footer
  int last = h_size + 1 + name_l + esc + 6;
  p[last] = xor; // xor

  return last + 1;
}

int _create_packet_1(unsigned char* p, unsigned char g_id, unsigned char p_id, unsigned int p_init_size, unsigned int x_max, unsigned int y_max, unsigned int time_lim, unsigned int n_lives) {
  unsigned char type = 1;
  unsigned int npk = 0;
  unsigned char xor = 0;
  unsigned char N_LEN = 1 + 1 + 4*5; // 2 times 1 byte, 5 times 4 bytes
  int esc = 0; // escape count

  // p header
  int h_size = 11;
  esc += set_packet_header(type, p, N_LEN, npk, &xor);

  // p data
  xor ^= g_id; xor ^= p_id;
  esc += escape_assign(g_id, &p[h_size + esc]); // game id
  esc += escape_assign(p_id, &p[h_size + esc + 1]); // player id
  esc += process_int_lendian(p_init_size, &p[h_size + esc + 2], &xor); // player init size
  esc += process_int_lendian(x_max, &p[h_size + esc + 6], &xor); // field's x_max
  esc += process_int_lendian(y_max, &p[h_size + esc + 10], &xor); // field's y_max
  esc += process_int_lendian(time_lim, &p[h_size + esc + 14], &xor); // time limit
  esc += process_int_lendian(n_lives, &p[h_size + esc + 18], &xor); // num_of_lives

  // p footer
  int last = h_size + esc + 22;
  p[last] = xor; // xor

  return last + 1;
}

int _create_packet_2(unsigned char* p, unsigned char g_id, unsigned char p_id, unsigned char r_char) {
  unsigned char type = 2, byte3 = 0;
  unsigned int npk = 0;
  unsigned char xor = 0;
  unsigned char N_LEN = 1 + 1 + 1;
  int esc = 0, ready = 0;
  if (r_char == 1) ready = 1;

  // p header
  int h_size = 11;
  esc += set_packet_header(type, p, N_LEN, npk, &xor);

  // p data
  xor ^= g_id; xor ^= p_id;
  esc += escape_assign(g_id, &p[h_size + esc]); // game id
  esc += escape_assign(p_id, &p[h_size + esc + 1]); // player id
  byte3 = byte3 | (ready << 2);
  xor ^= byte3;
  esc += escape_assign(byte3, &p[h_size+esc+2]);
  // more optional packet settings might go here...

  // p footer
  int last = h_size+esc+3;
  p[last] = xor; // xor

  return last + 1;
}


int _packet3_helper_process_dots(dot** dots, unsigned short int n_dots, unsigned char* p_part, unsigned char* xor){
  int esc = 0, bytesWritten = 0;
  unsigned int x,y;

  for(int i=0; i < n_dots; ++i) {
    if(dots[i]) {
      esc = 0;
      x = dots[i]->x;
      y = dots[i]->y;

      esc += process_int_lendian(x, &p_part[esc + bytesWritten], xor);
      esc += process_int_lendian(y, &p_part[esc + bytesWritten + 4], xor);

      bytesWritten += esc + 4 + 4;
    } else {
      printf("[WARNING] Hmm.. dot was not initialized...\n");
    }
  }

  return bytesWritten - 8*n_dots;
}

int _packet3_helper_process_clients(client_struct** clients, int* n_clients, unsigned char* p_data, int* clients_total_len, unsigned char *xor) {
  unsigned char ID, name_l;
  unsigned int x, y, size, score, lives, bytesWritten = 0;
  int esc_internal = 0, esc_total = 0;
  char username[256], color[7];

  for(int i=0; i < MAX_CLIENTS; ++i) {
		if(clients[i] && clients[i]->has_introduced) {
      esc_internal = 0;
      ID = clients[i]->ID;
			x = clients[i]->x;
			y = clients[i]->y;
			size = clients[i]->size;
			score = clients[i]->score;
			lives = clients[i]->lives;
      strcpy(username, clients[i]->username);
      name_l = strlen(username);
      strcpy(color, clients[i]->color);

      (*n_clients)++;
      (*xor) ^= ID; (*xor) ^= name_l;
      esc_internal += escape_assign(ID, &p_data[0 + esc_internal + bytesWritten]); // ID (1 byte)
      esc_internal += escape_assign(name_l, &p_data[1 + esc_internal + bytesWritten]); // Length of name (1 byte)
      esc_internal += process_str(username, xor, &p_data[2 + esc_internal + bytesWritten]); // username (strlen(username) bytes)
      esc_internal += process_int_lendian(x, &p_data[2 + esc_internal + name_l + bytesWritten], xor); // x location (4 bytes)
      esc_internal += process_int_lendian(y, &p_data[6 + esc_internal + name_l + bytesWritten], xor); // y location (4 bytes)
      esc_internal += process_str(color, xor, &p_data[10 + esc_internal + name_l + bytesWritten]); // color (6 bytes)
      esc_internal += process_int_lendian(size, &p_data[16 + esc_internal + name_l + bytesWritten], xor); // size (4 bytes)
      esc_internal += process_int_lendian(score, &p_data[20 + esc_internal + name_l + bytesWritten], xor); // score (4 bytes)
      esc_internal += process_int_lendian(lives, &p_data[24 + esc_internal + name_l + bytesWritten], xor); // lives (4 bytes);

      esc_total += esc_internal;

      bytesWritten += 28 + esc_internal + name_l;

		}
	}

  *clients_total_len = bytesWritten;

  return esc_total; // used only for N_LEN
}

int _create_packet_3(unsigned char* p, unsigned char g_id, client_struct** clients, unsigned short int n_dots, dot** dots, unsigned int time_left) {
  unsigned char p_user_data[MAX_PACKET_SIZE];
  const unsigned char type = 3;
  unsigned char xor = 0;
  int esc = 0, c_total_len = 0, client_count = 0;
  int esc_cl = _packet3_helper_process_clients(clients, &client_count, p_user_data, &c_total_len, &xor); // process all clients
  const unsigned char N_LEN = (c_total_len - esc_cl) + 1 + 2 + 2 + 8*n_dots + 4;

  // should be implemented real
  unsigned int npk = 0;

  // p header
  int h_size = 11;
  esc += set_packet_header(type, p, N_LEN, npk, &xor);

  // p data
  xor ^= g_id;
  esc += escape_assign(g_id, &p[h_size + esc]); // game id (1 byte)
  esc += process_short_int_lendian(client_count, &p[h_size + esc + 1], &xor); // number of clients (2 bytes)
  memcpy(&p[h_size + esc + 3], p_user_data, c_total_len);
  esc += process_short_int_lendian(n_dots, &p[h_size + esc + 3 + c_total_len], &xor); // number of dots (2 bytes)
  esc += _packet3_helper_process_dots(dots, n_dots, &p[h_size + esc + 3 + c_total_len + 2], &xor); // process all dots (each 8 bytes)
  esc += process_int_lendian(time_left, &p[h_size + esc + 3 + c_total_len + 2 + n_dots*8], &xor); //time left (4 bytes)

  // p footer
  int last = h_size + esc + 3 + c_total_len + 2 + n_dots*8 + 4;
  p[last] = xor; // xor

  return last + 1;
}


int _create_packet_4(unsigned char *p, unsigned char *g_id, unsigned char *p_id, char w, char a, char s, char d) {
  // chars w,a,s,d each represents whether key is pressed... 1 - pressed, 0 - not pressed
  unsigned char type = 4, byte3 = 0;
  unsigned char xor = 0;
  unsigned char N_LEN = 1 + 1 + 1;
  int esc = 0;

  // should be implemented real npk
  unsigned int npk = 0;

  // p header
  int h_size = 11;
  esc += set_packet_header(type, p, N_LEN, npk, &xor);

  // p data
  xor ^= *g_id; xor ^= *p_id;
  esc += escape_assign(*g_id, &p[h_size + esc]); // game id
  esc += escape_assign(*p_id, &p[h_size + esc + 1]); // player id
  byte3 = byte3 | (w << 3);
  byte3 = byte3 | (a << 2);
  byte3 = byte3 | (s << 1);
  byte3 = byte3 | d;
  xor ^= byte3;
  esc += escape_assign(byte3, &p[h_size+esc+2]);

  // p footer
  int last = h_size+esc+3;
  p[last] = xor; // xor

  return last + 1;
}


int _create_packet_5(unsigned char* p, unsigned char g_id, unsigned char p_id, unsigned int score, unsigned int time_passed) {
  unsigned char type = 5;
  unsigned int npk = 0;
  unsigned char xor = 0;
  unsigned char N_LEN = 1 + 1 + 4 + 4;
  int esc = 0;

  // p header
  int h_size = 11;
  esc += set_packet_header(type, p, N_LEN, npk, &xor);

  // p data
  xor ^= g_id; xor ^= p_id;
  esc += escape_assign(g_id, &p[h_size + esc]); // game id
  esc += escape_assign(p_id, &p[h_size + esc + 1]); // player id
  esc += process_int_lendian(score, &p[h_size + esc + 2], &xor); // score
  esc += process_int_lendian(time_passed, &p[h_size + esc + 6], &xor); // score

  // p footer
  int last = h_size+esc+10;
  p[last] = xor; // xor

  return last + 1;
}

int _packet6_helper_process_clients(client_struct** clients, int* n_clients, unsigned char* p_data, int* clients_total_len, unsigned char *xor) { // process all clients
  unsigned char name_l;
  unsigned int score, bytesWritten = 0;
  int esc_internal = 0, esc_total = 0;
  char username[256];

  for(int i=0; i < MAX_CLIENTS; ++i) {
		if(clients[i] && clients[i]->has_introduced) {
      esc_internal = 0;
			score = clients[i]->score;
      strcpy(username, clients[i]->username);
      name_l = strlen(username);

      (*n_clients)++;
      (*xor) ^= name_l;
      esc_internal += escape_assign(name_l, &p_data[0 + esc_internal + bytesWritten]); // Length of name (1 byte)
      esc_internal += process_str(username, xor, &p_data[1 + esc_internal + bytesWritten]); // username (strlen(username) bytes)
      esc_internal += process_int_lendian(score, &p_data[1 + esc_internal + bytesWritten + name_l], xor); // score (4 bytes)

      esc_total += esc_internal;

      bytesWritten += 5 + esc_internal + name_l;
		}
	}

  *clients_total_len = bytesWritten;

  return esc_total; // used only for N_LEN
}

int _create_packet_6(unsigned char* p, unsigned char g_id, client_struct** clients, unsigned char curr_player_id, unsigned int curr_player_score) {
  unsigned char p_user_data[MAX_PACKET_SIZE];
  const unsigned char type = 6;
  unsigned char xor = 0;
  int esc = 0, c_total_len = 0, client_count = 0;
  int esc_cl = _packet6_helper_process_clients(clients, &client_count, p_user_data, &c_total_len, &xor); // process all clients
  const unsigned char N_LEN = 1 + 1 + 4 + 2 + (c_total_len - esc_cl);
  unsigned int npk = 0;

  // p header
  int h_size = 11;
  esc += set_packet_header(type, p, N_LEN, npk, &xor);

  // p data
  xor ^= g_id; xor^= curr_player_id;
  esc += escape_assign(g_id, &p[h_size + esc]); // game id (1 byte)
  esc += escape_assign(curr_player_id, &p[h_size + esc + 1]); // receiver's (current player's) ID (1 byte)
  esc += process_int_lendian(curr_player_score, &p[h_size + esc + 2], &xor); // receiver's (current player's) Score (4 bytes)
  esc += process_short_int_lendian(client_count, &p[h_size + esc + 6], &xor); // number of clients (2 bytes)
  memcpy(&p[h_size + esc + 8], p_user_data, c_total_len);

  // p footer
  int last = h_size + esc + 8 + c_total_len;
  p[last] = xor; // xor

  return last + 1;
}

int _create_packet_7(unsigned char* p, unsigned char g_id, unsigned char p_id, char* message) {
  const unsigned char type = 7;
  unsigned char xor = 0;
  unsigned int npk = 0, message_l = strlen(message);
  int esc = 0;
  const unsigned char N_LEN = message_l + 4;

  // p header
  int h_size = 11;
  esc += set_packet_header(type, p, N_LEN, npk, &xor);

  // p data
  xor ^= g_id; xor^= p_id;
  esc += escape_assign(g_id, &p[h_size + esc]); // game id (1 byte)
  esc += escape_assign(p_id, &p[h_size + esc + 1]); // player id ID (1 byte)
  esc += process_short_int_lendian(message_l, &p[h_size + esc + 2], &xor); // message length (2 bytes)
  // memcpy + xor
  int i;
  for (i = 0; i < message_l; i++) {
    xor ^= message[i];
    p[h_size + esc + 4 + i] = message[i];
  }

  // p footer
  int last = h_size + esc + 4 + message_l;
  p[last] = xor; // xor

  return last + 1;
}

int process_packet_0(unsigned char* p_dat, client_struct* client) {
  unsigned char name_l = (unsigned char) p_dat[0];
  char username[256] = {0}, color[7] = {0};
  memcpy(username, &p_dat[1], name_l);
  memcpy(color, &p_dat[1 + name_l], 6);

  client->has_introduced = 1;
  strcpy(client->username, username);
  strcpy(client->color, color);

  // maybe should check if username already exists

  if (name_l != strlen(username) || strlen(color) != 6) {
    printf("[ERROR] err processing packet 0. \n");
    printf("username[%ld=%d] = %s, color = %s\n", strlen(username), name_l, username, color);
    return -1;
  }
  // printf("[OK] Process packet function assigned: username = %s, color = %s\n", username, color);
  printf("[OK] Client username = %s, client color = %s (cstruct)\n", client->username, client->color);

  // sending packet 1
  unsigned char p[MAX_PACKET_SIZE];
  unsigned char p_id = (unsigned char) client->ID;
  unsigned char g_id = 211; // should be evaluated somehow

  int packet_size = _create_packet_1(p, g_id, p_id, INIT_SIZE, MAX_X, MAX_Y, TIME_LIM, INIT_LIVES);

  if (send_prepared_packet(p, packet_size, client->socket) < 0) {
    printf("[ERROR] Packet 1 could not be sent.\n");
      return -1;
  } else {
    printf("[OK] Packet 1 sent successfully.\n");
  }

  return 0;
}

int process_packet_1(unsigned char* p_dat, int c_socket, int *client_status, unsigned char* g_id, unsigned char* p_id) {
  *client_status = 1;
  *g_id = p_dat[0];
  *p_id = p_dat[1];
  unsigned int p_init_size = get_int_from_4bytes_lendian(&p_dat[2]); // 4 bytes
  unsigned int max_x = get_int_from_4bytes_lendian(&p_dat[6]); // 4 bytes
  unsigned int max_y = get_int_from_4bytes_lendian(&p_dat[10]); // 4 bytes
  unsigned int time_limit = get_int_from_4bytes_lendian(&p_dat[14]); // 4 bytes
  unsigned int num_of_lives = get_int_from_4bytes_lendian(&p_dat[18]); // 4 bytes

  printf("[OK] Got packet 1. Game ID = %d, Player ID = %d, p_init_size = %d, field size (%d, %d), time %d, lives %d\n",
          *g_id, *p_id, p_init_size, max_x, max_y, time_limit, num_of_lives);

  // wait for user input to send ready message
  *client_status = 2;
  printf("Write anything to send ready message:\n");
  // fgetc in send thread. It will change client_status to 3 when input received
  // if current client status is 2.
  while (*client_status != 3) {
    sleep(1);
  }

  char ready = 1;

  // sending packet 2
  unsigned char p[MAX_PACKET_SIZE];
  int packet_size = _create_packet_2(p, *g_id, *p_id, ready);

  if (send_prepared_packet(p, packet_size, c_socket) < 0) {
    printf("[ERROR] Packet 2 could not be sent.\n");
      return -1;
  } else {
    printf("[OK] Packet 2 sent successfully.\n");
  }

  *client_status = 4;

  return 0;
}

int process_packet_2(unsigned char* p_dat, client_struct* client) {
  unsigned char g_id, p_id, byte3;
  int ready;
  g_id = p_dat[0];
  p_id = p_dat[1];
  byte3 = p_dat[2];
  int ready_bitNumber = 2; // from left
  ready = get_bit(byte3, ready_bitNumber);

  printf("Received packet 2. g_id = %d, p_id = %d, byte 3 = %d, ready = %d\n", g_id, p_id, byte3, ready);

  client->ready = ready;
  return 0;
}

void trash_function(void* arg1, void *arg2, void* arg3, void* arg4, void* arg5) {}

int process_packet_3(unsigned char* p, int c_socket, int* client_status) {
  unsigned char g_id;
  unsigned short int n_players, n_dots;
  unsigned int time_left;
  client_struct* clients[MAX_CLIENTS];

  char username[256] = {0}, color[7] = {0};
  int name_l, total_client_len = 0;

  printf("[OK] Packet 3 received.\n");

  g_id = p[0];
  n_players = get_int_from_4bytes_lendian(&p[1]); // 2 byte int

  // get PLAYER DATA
  printf("[OK] The following clients were received by packet3: \n");
  for(int i=0; i < n_players; ++i) {
    // malloc client
    client_struct* client = (client_struct *) malloc(sizeof(client_struct));
    if (client == NULL) {
      printf("[WARNING] Malloc did not work. Cannot create client in packet receiving.\n");
      return -1;
    }

    client->ID = p[3 + total_client_len];
    name_l = p[4 + total_client_len];
    memcpy(client->username, &p[5 + total_client_len], name_l);
    client->x = get_int_from_4bytes_lendian(&p[5 + name_l + total_client_len]); // 4 bytes
    client->y = get_int_from_4bytes_lendian(&p[9 + name_l + total_client_len]); // 4 bytes
    memcpy(client->color, &p[13 + total_client_len + name_l], 6);
    client->size = get_int_from_4bytes_lendian(&p[19 + total_client_len + name_l]); // 4 bytes
    client->score = get_int_from_4bytes_lendian(&p[23 + total_client_len + name_l]); // 4 bytes
    client->lives = get_int_from_4bytes_lendian(&p[27 + total_client_len + name_l]); // 4 bytes

    clients[i] = client;

    printf("Client %d: %s (id=%d, #%s), x=%d, y=%d, size=%d, score=%d, lives=%d \n",
      i+1, client->username, client->ID, client->color, client->x, client->y, client->size, client->score, client->lives);

      total_client_len += 30 + name_l - 3 + 1;
  }

  n_dots = get_sh_int_2bytes_lendian(&p[3 + total_client_len]); // 2 bytes
  dot* dots[n_dots];

  // get DOT DATA
  for(int i=0; i < n_dots; ++i) {
    // malloc dot
    dot* somedot = (dot *) malloc(sizeof(dot));
    if (somedot == NULL) {
      printf("[WARNING] Malloc did not work. Cannot create dot in packet receiving.\n");
      return -1;
    }

    somedot->x = get_int_from_4bytes_lendian(&p[3 + total_client_len + 2 + i*8]); // 4 bytes
    somedot->y = get_int_from_4bytes_lendian(&p[3 + total_client_len + 2 + i*8 + 4]); // 4 bytes

    dots[i] = somedot;
  }

  // print DOT DATA
  printf("[OK] Got the following dots: ");
  for(int i=0; i < n_dots; ++i) {
    if (i == n_dots - 1) printf("[%d, %d]\n", dots[i]->x, dots[i]->y);
    else printf("[%d, %d], ", dots[i]->x, dots[i]->y);
  }

  time_left = get_int_from_4bytes_lendian(&p[3 + total_client_len + 2 + n_dots*8]); // 4 bytes

  printf("[OK] time left: %d\n", time_left);

  trash_function(dots, clients, &g_id, username, color); // used to disable warnings not useful

  return 0;
}

int process_packet_4(unsigned char* p_dat, client_struct* client) {
  unsigned char g_id, p_id, byte3;
  char w, a, s, d;

  char w_pos = 3, a_pos = 2, s_pos = 1, d_pos = 0; // bitNumber, see get_bit
  g_id = p_dat[0];
  p_id = p_dat[1];
  byte3 = p_dat[2];
  w = get_bit(byte3, w_pos);
  a = get_bit(byte3, a_pos);
  s = get_bit(byte3, s_pos);
  d = get_bit(byte3, d_pos);

  printf("Received packet 4. g_id = %d, p_id = %d, pressed(w,a,s,d)=(%d,%d,%d,%d)\n", g_id, p_id, w, a, s, d);

  // some game logic should come here...

  return 0;
}

int process_packet_5(unsigned char* p_dat, int* client_status) {
  unsigned char g_id, p_id;
  g_id = p_dat[0];
  p_id = p_dat[1];

  unsigned int score, time_passed;
  score = get_int_from_4bytes_lendian(&p_dat[2]); // 4 bytes
  time_passed = get_int_from_4bytes_lendian(&p_dat[6]); // 4 bytes

  printf("[OK] Received packet 5. You died. Your score: %d, time passed %d. g_id=%d, p_id=%d\n", score, time_passed, g_id, p_id);

  *client_status = 5;

  return 0;
}

int process_packet_6(unsigned char* p_dat, int* client_status) {
  unsigned char g_id, p_id;
  unsigned short int n_players;
  g_id = p_dat[0];
  p_id = p_dat[1];

  unsigned int score;
  score = get_int_from_4bytes_lendian(&p_dat[2]); // 4 bytes
  n_players = get_int_from_4bytes_lendian(&p_dat[6]); // 2 bytes

  printf("[OK] Received packet 6. GAME ENDED. g_id=%d, p_id=%d, p_score=%d\n", g_id, p_id, score);

  int i;
  unsigned int bytesRead = 0;
  printf("==== LEADERBOARD =====\n");
  for (i = 0; i < n_players; i++) {
    unsigned char name_l;
    char username[256] = {0};
    unsigned int score;

    name_l = p_dat[8 + bytesRead]; // 1 byte
    memcpy(username, &p_dat[8 + bytesRead + 1], name_l); // name_l bytes
    score = get_int_from_4bytes_lendian(&p_dat[8 + bytesRead + 1 + name_l]); // 4 bytes

    bytesRead += 1 + name_l + 4;

    printf("%s has %d points\n", username, score);
  }

  *client_status = 6;

  return 0;
}


int process_packet_7 (unsigned char* p_dat) {
  unsigned char g_id, p_id;
  char message[MAX_MESSAGE_SIZE + 1] = {0};
  g_id = p_dat[0];
  p_id = p_dat[1];

  short int message_l;
  message_l = get_sh_int_2bytes_lendian(&p_dat[2]); // 2 bytes

  memcpy(message, &p_dat[4], message_l); // name_l bytes

  printf("Received new message: %s\n gid=%d,pid=%d\n", message, g_id, p_id);

  return 0;
}

void process_incoming_packet(unsigned char *p, int size_header, int size_data, client_struct *client, int c_socket, int *client_status, unsigned char *g_id, unsigned char *p_id, int is_server) {
  // is_server = 1 (called function in server), is server = 0 (called function in client)
  int size = size_header + size_data;

  int type = (int) p[0];

  unsigned char checksum = get_checksum(p, size - 1);
  if (p[size-1] == checksum)
    printf("[OK] Checksums are correct.\n");
  else {
    printf("[WARNING] Packet number %d: checksums mismatch :(. Abandoned the packet. In packet %d but in server %d. \n", type, p[size-1], checksum);
    return;
  }

  int process_result = -1;
  switch (type) {
    case 0:
      if (is_server) process_result = process_packet_0(&p[size_header], client);
      break;
    case 1:
      if (!is_server && size_data == 23) process_result = process_packet_1(&p[size_header], c_socket, client_status, g_id, p_id);
      break;
    case 2:
      if (is_server) process_result = process_packet_2(&p[size_header], client);
      break;
    case 3:
      if (!is_server) process_result = process_packet_3(&p[size_header], c_socket, client_status);
      break;
    case 4:
      if (is_server) process_result = process_packet_4(&p[size_header], client);
      break;
    case 5:
      if (!is_server) process_result = process_packet_5(&p[size_header], client_status);
      break;
    case 6:
      if (!is_server) process_result = process_packet_6(&p[size_header], client_status);
      break;
    case 7:
      print_bytes(&p[size_header], size - size_header);
      process_result = process_packet_7(&p[size_header]);
      break;
    default:
      printf("Invalid packet type. Abandoned.\n");
      process_result = -1;
      break;
  }

  if (process_result < 0) {
    printf("[WARNING] Packet could not be processed by type function. \n");
    return;
  };

}

int recv_byte (
  unsigned char* packet_in,
  int *packet_cursor,
  int *current_packet_data_size,
  int *packet_status,
  int is_server,
  client_struct *client,
  int client_socket,
  int *client_status,
  unsigned char *g_id,
  unsigned char *p_id
  ) {

  int receive, socket = client_socket;
  unsigned char rec_byte[1];
  int packet_header_size_excl_div = 9;

  if (client != NULL) socket = client->socket;

  if (*packet_cursor == 5) { // position just after N_LEN
      *current_packet_data_size = get_int_from_4bytes_lendian(&packet_in[1]);
      *packet_status = 2;
      printf("[OK] Got packet data size: %d\n", *current_packet_data_size);
  }

  if (*packet_cursor == packet_header_size_excl_div + *current_packet_data_size + 1) {
    // printf("[OK] Reached end of packet reading... Current cursor pos. %d\n", *packet_cursor);
    process_incoming_packet(packet_in, packet_header_size_excl_div, *current_packet_data_size + 1, client, socket, client_status, g_id, p_id, is_server); // TODO make seperate thread or smth so it can continue reading packets...
    *packet_status = 0;
    *current_packet_data_size = 0;
    *packet_cursor = 0;
  }

  receive = recv(socket, rec_byte, 1, 0);
	if (receive > 0) { // received byte
    if (rec_byte[0] == 0) { // divisor
      // print_one_byte(rec_byte[0]);
      receive = recv(socket, rec_byte, 1, 0);
      if (receive > 0) { // received successfully
        // print_one_byte(rec_byte[0]);
        if (rec_byte[0] == 0) { // new packet
          if (*packet_status > 0) {// previous packet should have been finished => error
            printf("[WARNING] SHOULD NOT HAPPEN.\n");
            bzero(packet_in, MAX_PACKET_SIZE);
            // continue;
          }
          *packet_status = 1;
          *current_packet_data_size = 0;
          *packet_cursor = 0;
          bzero(packet_in, MAX_PACKET_SIZE); // This could be improved - skip 0 and then strlen
          // continue;
        } else { // error
          bzero(packet_in, MAX_PACKET_SIZE); // clean the packet
          printf("[WARNING] Packet invalid. Contained 0. From socket %d. Packet dropped. \n", socket);
          return -1;
        }
      } else {
        printf("[WARNING] Could not second recv after 0. Socket %d\n", socket);
        return -1;
      }
    } else { // not divisor
      if (*packet_status == 0) { // should be started => here is error
        printf("[WARNING] Something wrong with packet [no. 2]. Socket %d\n", socket);
        printf("p_status = %d, byte = %c (%d), p_cursor = %d, p_data_Size = %d\n", *packet_status, printable_char(rec_byte[0]), rec_byte[0], *packet_cursor, *current_packet_data_size);
        bzero(packet_in, MAX_PACKET_SIZE);
        return -1;
      }

      if (rec_byte[0] == 1) {
        receive = recv(socket, rec_byte, 1, 0);
        if (receive > 0) { // received successfully
          if (rec_byte[0] == 2) { // new packet
              // print_one_byte(0);
              packet_in[*packet_cursor] = 0; // 12 is escaped 0
              (*packet_cursor)++;
              // continue;
          } else if (rec_byte[0] == 3) {
            // print_one_byte(1);
            packet_in[*packet_cursor] = 1; // 13 is escaped 1
            (*packet_cursor)++;
            // continue;
          } else { // error
            bzero(packet_in, MAX_PACKET_SIZE); // clean the packet
            printf("[WARNING] Packet invalid. Contained 1 and no following 2/3. From socket %d. Packet dropped. \n", socket);
            return -1;
          }
        } else {
          printf("[WARNING] Could not second recv after 1. Socket %d\n", socket);
          return -1;
        }
      } else {
        // print_one_byte(rec_byte[0]);
        packet_in[*packet_cursor] = rec_byte[0];
        (*packet_cursor)++;
      }
    }

  } else {
    if (client_status && (*client_status == 5 || *client_status == 6))
      return -1;
    printf("[WARNING] Error recv. Smth.\n");
    return -1;
  }

    return 1; // success
}

int set_packet_header(unsigned char type, unsigned char* p, unsigned int N_LEN, unsigned int npk, unsigned char* xor) {
  int esc = 0;

  p[0] = 0; p[1] = 0; // divider
  (*xor) ^= type;
  esc += escape_assign(type, &p[2]); // type
  esc += process_int_lendian(N_LEN, &p[3 + esc], xor); // N_LEN
  esc += process_int_lendian(npk, &p[7 + esc], xor); // npk

  return esc;
}

int send_prepared_packet(unsigned char* p, int p_size, int socket) {
  for (int i = 0; i < p_size; i++) {
    // printf("Sending packet's element %d: %c (%d)\n", i, printable_char(p[i]), p[i]);
  	if (send(socket, (p + i), 1, 0) < 0) return -1;
  }

  return 0;
}

int get_username_color(char* username, char* color) {
  printf("[DEV] Press enter to autofill.\n");

  char username_temp[260]; // actually max 255
  char color_temp[10]; // actually max 6
  // insert username
  int username_ok = 1;
  printf("Please enter your username: ");
  fgets(username_temp, 260, stdin);
  // autofill
  if (strcmp(username_temp, "\n\0") == 0) {
    printf("[DEV] Name: UsernameTest1, color: ABC12D. Username length 13.\n");
    strcpy(username, "UsernameTest1");
    strcpy(color, "ABC12D");
    return 0;
  }

  remove_newline(username_temp);
  if (strlen(username_temp) > 255 || strlen(username_temp) < 1) {
      printf("Username should be between 1 and 255 chars. \n");
      username_ok = 0;
  }

  while (!username_ok) {
    printf("\nPlease try again: ");
    fgets(username_temp, 260, stdin);
    remove_newline(username_temp);
    if (strlen(username_temp) > 255 || strlen(username_temp) < 1) {
      printf("Username should be between 1 and 255 chars. \n");
      username_ok = 0;
    } else {
      username_ok = 1;
    }
  }

  // insert color
  int color_ok = 1;
  printf("Please enter your color (6 hex digits): ");
  fgets(color_temp, 10, stdin);
  remove_newline(color_temp);
  if (strlen(color_temp) != 6) {
    printf("Color should be exactly 6 hex digits. \n");
    color_ok = 0;
  }

  char c;
  if ((c = contains_only_hex_digits(color_temp)) != -1) {
    printf("Color contains non-hex-digit character: %c\n", c);
    color_ok = 0;
  }

  while (!color_ok) {
    printf("\nPlease try again: ");
    fgets(color_temp, 10, stdin);
    remove_newline(color_temp);
    if (strlen(color_temp) != 6) {
      printf("Color should be exactly 6 hex digits. \n");
      color_ok = 0;
    } else if ((c = contains_only_hex_digits(color_temp)) != -1) {
      printf("Color contains non-hex-digit character: %c\n", c);
      color_ok = 0;
    } else {
      color_ok = 1;
    }
  }

  strcpy(username, username_temp);
  strcpy(color, color_temp);

  return 0;
}

int process_int_lendian(int n, unsigned char* p_part, unsigned char* xor) {
  int escape_count = 0;
  unsigned char byte;

  // byte 1
  byte = n & 0xFF;
  (*xor) ^= byte;
  escape_count += escape_assign(byte, &p_part[0]);
  // byte 2
  byte = (n >> 8) & 0xFF;
  (*xor) ^= byte;
  escape_count += escape_assign(byte, &p_part[1 + escape_count]);
  // byte 3
  byte = (n >> 16) & 0xFF;
  (*xor) ^= byte;
  escape_count += escape_assign(byte, &p_part[2 + escape_count]);
  // byte 4
  byte = (n >> 24) & 0xFF;
  (*xor) ^= byte;
  escape_count += escape_assign(byte, &p_part[3 + escape_count]);

  return escape_count;
}

int process_short_int_lendian(short int n, unsigned char* p_part, unsigned char* xor) {
  int escape_count = 0;
  unsigned char byte;

  // byte 1
  byte = n & 0xFF;
  (*xor) ^= byte;
  escape_count += escape_assign(byte, &p_part[0]);
  // byte 2
  byte = (n >> 8) & 0xFF;
  (*xor) ^= byte;
  escape_count += escape_assign(byte, &p_part[1 + escape_count]);

  return escape_count;
}

int process_str(char* str, unsigned char* xor, unsigned char* p_part) {
  int i, str_l = strlen(str), esc = 0;
  for (i = 0; i < str_l; i++) {
    (*xor) ^= str[i];
    esc += escape_assign(str[i], &p_part[i+esc]);
  }

  return esc;
}
