#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h> /* toupper, isprint */
#include <inttypes.h>
#include <unistd.h>
#include "util_functions.h"
#include "setup.h"
#include "functions.h"

int _create_packet_0(unsigned char* p, char* name, char* color) {
  unsigned char type = 0;
  unsigned int npk = 0;
  unsigned char xor = 0;
  unsigned char name_l = (unsigned char) strlen(name);
  unsigned int N_LEN = name_l + 1 + 6; /* 1 - name_l, 6 - color */
  int esc = 0; /* escape count */

  /* p header */
  int h_size = 11;
  esc += set_packet_header(type, p, N_LEN, npk, &xor);

  /* p data */
  xor ^= name_l;
  esc += escape_assign(name_l, &p[h_size + esc]); /* name_l */
  esc += process_str(name, &xor, &p[h_size + 1 + esc]); /* name */
  esc += process_str(color, &xor, &p[h_size + 1 + name_l + esc]); /* color */

  /* p footer */
  int last = h_size + 1 + name_l + esc + 6;
  p[last] = xor; /* xor */
  p[last+1] = 0;
  p[last+2] = 0;

  return last + 3;
}

int _create_packet_1(unsigned char* p, unsigned char g_id, unsigned char p_id, unsigned int p_init_size, unsigned int x_max, unsigned int y_max, unsigned int time_lim, unsigned int n_lives) {
  unsigned char type = 1;
  unsigned int npk = 0;
  unsigned char xor = 0;
  unsigned int N_LEN = 1 + 1 + 4*5; /* 2 times 1 byte, 5 times 4 bytes */
  int esc = 0; /* escape count */

  /* p header */
  int h_size = 11;
  esc += set_packet_header(type, p, N_LEN, npk, &xor);

  /* p data */
  xor ^= g_id; xor ^= p_id;
  esc += escape_assign(g_id, &p[h_size + esc]); /* game id */
  esc += escape_assign(p_id, &p[h_size + esc + 1]); /* player id */
  esc += process_int_lendian(p_init_size, &p[h_size + esc + 2], &xor); /* player init size */
  esc += process_int_lendian(x_max, &p[h_size + esc + 6], &xor); /* field's x_max */
  esc += process_int_lendian(y_max, &p[h_size + esc + 10], &xor); /* field's y_max */
  esc += process_int_lendian(time_lim, &p[h_size + esc + 14], &xor); /* time limit */
  esc += process_int_lendian(n_lives, &p[h_size + esc + 18], &xor); /* num_of_lives */

  /* p footer */
  int last = h_size + esc + 22;
  p[last] = xor; /* xor */
  p[last+1] = 0;
  p[last+2] = 0;

  return last + 3;
}

int _create_packet_2(unsigned char* p, unsigned char g_id, unsigned char p_id, unsigned char r_char) {
  unsigned char type = 2, byte3 = 0;
  unsigned int npk = 0;
  unsigned char xor = 0;
  unsigned int N_LEN = 1 + 1 + 1;
  int esc = 0, ready = 0;
  if (r_char == 1) ready = 1;

  /* p header */
  int h_size = 11;
  esc += set_packet_header(type, p, N_LEN, npk, &xor);

  /* p data */
  xor ^= g_id; xor ^= p_id;
  esc += escape_assign(g_id, &p[h_size + esc]); /* game id */
  esc += escape_assign(p_id, &p[h_size + esc + 1]); /* player id */
  byte3 = byte3 | (ready << 2);
  xor ^= byte3;
  esc += escape_assign(byte3, &p[h_size+esc+2]);
  /* more optional packet settings might go here... */

  /* p footer */
  int last = h_size+esc+3;
  p[last] = xor; /* xor */
  p[last+1] = 0;
  p[last+2] = 0;

  return last + 3;
}


int _packet3_helper_process_dots(dot** dots, unsigned short int n_dots, unsigned char* p_part, unsigned char* xor){
  int esc = 0, bytesWritten = 0;
  unsigned int x,y;

  int i;
  for(i=0; i < MAX_DOTS; ++i) {
    if(dots[i]) {
      esc = 0;
      x = dots[i]->x;
      y = dots[i]->y;

      esc += process_int_lendian(x, &p_part[esc + bytesWritten], xor);
      esc += process_int_lendian(y, &p_part[esc + bytesWritten + 4], xor);

      bytesWritten += esc + 4 + 4;
    } else {
      /* printf("[WARNING] Hmm.. dot was not initialized...\n"); */
    }
  }

  return bytesWritten - 8*n_dots;
}

int _packet3_helper_process_clients(client_struct** clients, int* n_clients, unsigned char* p_data, int* clients_total_len, unsigned char *xor) {
  unsigned char ID, name_l;
  unsigned int x, y, size, score, lives, bytesWritten = 0;
  int esc_internal = 0, esc_total = 0;
  char username[256], color[7];

  int i;
  for(i=0; i < MAX_CLIENTS; ++i) {
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
        esc_internal += escape_assign(ID, &p_data[0 + esc_internal + bytesWritten]); /* ID (1 byte) */
        esc_internal += escape_assign(name_l, &p_data[1 + esc_internal + bytesWritten]); /* Length of name (1 byte) */
        esc_internal += process_str(username, xor, &p_data[2 + esc_internal + bytesWritten]); /* username (strlen(username) bytes) */
        esc_internal += process_int_lendian(x, &p_data[2 + esc_internal + name_l + bytesWritten], xor); /* x location (4 bytes) */
        esc_internal += process_int_lendian(y, &p_data[6 + esc_internal + name_l + bytesWritten], xor); /* y location (4 bytes) */
        esc_internal += process_str(color, xor, &p_data[10 + esc_internal + name_l + bytesWritten]); /* color (6 bytes) */
        esc_internal += process_int_lendian(size, &p_data[16 + esc_internal + name_l + bytesWritten], xor); /* size (4 bytes) */
        esc_internal += process_int_lendian(score, &p_data[20 + esc_internal + name_l + bytesWritten], xor); /* score (4 bytes) */
        esc_internal += process_int_lendian(lives, &p_data[24 + esc_internal + name_l + bytesWritten], xor); /* lives (4 bytes); */

        esc_total += esc_internal;

        bytesWritten += 28 + esc_internal + name_l;

		}
	}

  *clients_total_len = bytesWritten;

  return esc_total; /* used only for N_LEN */
}

int _create_packet_3(unsigned char* p, unsigned char g_id, client_struct** clients, unsigned short int n_dots, dot** dots, unsigned int time_left, unsigned int npk) {
  unsigned char p_user_data[MAX_PACKET_SIZE];
  const unsigned char type = 3;
  unsigned char xor = 0;
  int esc = 0, c_total_len = 0, client_count = 0;

  int esc_cl = _packet3_helper_process_clients(clients, &client_count, p_user_data, &c_total_len, &xor); /* process all clients */
  const unsigned int N_LEN = (c_total_len - esc_cl) + 1 + 2 + 2 + 8*n_dots + 4;

  /* p header */
  int h_size = 11;
  esc += set_packet_header(type, p, N_LEN, npk, &xor);

  /* p data */
  xor ^= g_id;
  esc += escape_assign(g_id, &p[h_size + esc]); /* game id (1 byte) */
  esc += process_short_int_lendian(client_count, &p[h_size + esc + 1], &xor); /* number of clients (2 bytes) */
  memcpy(&p[h_size + esc + 3], p_user_data, c_total_len);
  esc += process_short_int_lendian(n_dots, &p[h_size + esc + 3 + c_total_len], &xor); /* number of dots (2 bytes) */
  esc += _packet3_helper_process_dots(dots, n_dots, &p[h_size + esc + 3 + c_total_len + 2], &xor); /* process all dots (each 8 bytes) */
  esc += process_int_lendian(time_left, &p[h_size + esc + 3 + c_total_len + 2 + n_dots*8], &xor); /*time left (4 bytes) */

  /* p footer */
  int last = h_size + esc + 3 + c_total_len + 2 + n_dots*8 + 4;
  p[last] = xor; /* xor */
  p[last+1] = 0;
  p[last+2] = 0;

  return last + 3;
}


int _create_packet_4(unsigned char *p, unsigned char *g_id, unsigned char *p_id, char w, char a, char s, char d, unsigned int npk) {
  /* chars w,a,s,d each represents whether key is pressed... 1 - pressed, 0 - not pressed */
  unsigned char type = 4, byte3 = 0;
  unsigned char xor = 0;
  unsigned int N_LEN = 1 + 1 + 1;
  int esc = 0;

  /* p header */
  int h_size = 11;
  esc += set_packet_header(type, p, N_LEN, npk, &xor);

  /* p data */
  xor ^= *g_id; xor ^= *p_id;
  esc += escape_assign(*g_id, &p[h_size + esc]); /* game id */
  esc += escape_assign(*p_id, &p[h_size + esc + 1]); /* player id */
  byte3 = byte3 | (w << 3);
  byte3 = byte3 | (a << 2);
  byte3 = byte3 | (s << 1);
  byte3 = byte3 | d;
  xor ^= byte3;
  esc += escape_assign(byte3, &p[h_size+esc+2]);

  /* p footer */
  int last = h_size+esc+3;
  p[last] = xor; /* xor */
  p[last+1] = 0;
  p[last+2] = 0;

  return last + 3;
}


int _create_packet_5(unsigned char* p, unsigned char g_id, unsigned char p_id, unsigned int score, unsigned int time_passed) {
  unsigned char type = 5;
  unsigned int npk = 0;
  unsigned char xor = 0;
  unsigned int N_LEN = 1 + 1 + 4 + 4;
  int esc = 0;

  /* p header */
  int h_size = 11;
  esc += set_packet_header(type, p, N_LEN, npk, &xor);

  /* p data */
  xor ^= g_id; xor ^= p_id;
  esc += escape_assign(g_id, &p[h_size + esc]); /* game id */
  esc += escape_assign(p_id, &p[h_size + esc + 1]); /* player id */
  esc += process_int_lendian(score, &p[h_size + esc + 2], &xor); /* score */
  esc += process_int_lendian(time_passed, &p[h_size + esc + 6], &xor); /* score */

  /* p footer */
  int last = h_size+esc+10;
  p[last] = xor; /* xor */
  p[last+1] = 0;
  p[last+2] = 0;

  return last + 3;
}

int _packet6_helper_process_clients(client_struct** clients, int* n_clients, unsigned char* p_data, int* clients_total_len, unsigned char *xor) { /* process all clients */
  unsigned char name_l;
  unsigned int score, bytesWritten = 0;
  int esc_internal = 0, esc_total = 0;
  char username[256];

  int i;
  for(i=0; i < MAX_CLIENTS; ++i) {
		if(clients[i] && clients[i]->has_introduced) {
      esc_internal = 0;
			score = clients[i]->score;
      strcpy(username, clients[i]->username);
      name_l = strlen(username);

      (*n_clients)++;
      (*xor) ^= name_l;
      esc_internal += escape_assign(name_l, &p_data[0 + esc_internal + bytesWritten]); /* Length of name (1 byte) */
      esc_internal += process_str(username, xor, &p_data[1 + esc_internal + bytesWritten]); /* username (strlen(username) bytes) */
      esc_internal += process_int_lendian(score, &p_data[1 + esc_internal + bytesWritten + name_l], xor); /* score (4 bytes) */

      esc_total += esc_internal;

      bytesWritten += 5 + esc_internal + name_l;
		}
	}

  *clients_total_len = bytesWritten;

  return esc_total; /* used only for N_LEN */
}

int _create_packet_6(unsigned char* p, unsigned char g_id, client_struct** clients, unsigned char curr_player_id, unsigned int curr_player_score) {
  unsigned char p_user_data[MAX_PACKET_SIZE];
  const unsigned char type = 6;
  unsigned char xor = 0;
  int esc = 0, c_total_len = 0, client_count = 0;
  int esc_cl = _packet6_helper_process_clients(clients, &client_count, p_user_data, &c_total_len, &xor); /* process all clients */
  const unsigned int N_LEN = 1 + 1 + 4 + 2 + (c_total_len - esc_cl);
  unsigned int npk = 0;

  /* p header */
  int h_size = 11;
  esc += set_packet_header(type, p, N_LEN, npk, &xor);

  /* p data */
  xor ^= g_id; xor^= curr_player_id;
  esc += escape_assign(g_id, &p[h_size + esc]); /* game id (1 byte) */
  esc += escape_assign(curr_player_id, &p[h_size + esc + 1]); /* receiver's (current player's) ID (1 byte) */
  esc += process_int_lendian(curr_player_score, &p[h_size + esc + 2], &xor); /* receiver's (current player's) Score (4 bytes) */
  esc += process_short_int_lendian(client_count, &p[h_size + esc + 6], &xor); /* number of clients (2 bytes) */
  memcpy(&p[h_size + esc + 8], p_user_data, c_total_len);

  /* p footer */
  int last = h_size + esc + 8 + c_total_len;
  p[last] = xor; /* xor */
  p[last+1] = 0;
  p[last+2] = 0;

  return last + 3;
}

int _create_packet_7(unsigned char* p, unsigned char g_id, unsigned char p_id, char* message) {
  const unsigned char type = 7;
  unsigned char xor = 0;
  unsigned int npk = 0, message_l = strlen(message);
  int esc = 0;
  const unsigned int N_LEN = message_l + 4;

  /* p header */
  int h_size = 11;
  esc += set_packet_header(type, p, N_LEN, npk, &xor);

  /* p data */
  xor ^= g_id; xor^= p_id;
  esc += escape_assign(g_id, &p[h_size + esc]); /* game id (1 byte) */
  esc += escape_assign(p_id, &p[h_size + esc + 1]); /* player id ID (1 byte) */
  esc += process_short_int_lendian(message_l, &p[h_size + esc + 2], &xor); /* message length (2 bytes) */
  /* memcpy + xor */
  unsigned int i;
  for (i = 0; i < message_l; i++) {
    xor ^= message[i];
    p[h_size + esc + 4 + i] = message[i];
  }

  /* p footer */
  int last = h_size + esc + 4 + message_l;
  p[last] = xor; /* xor */
  p[last+1] = 0;
  p[last+2] = 0;

  return last + 3;
}

int process_packet_0(unsigned char* p_dat, client_struct* client, game *current_game) {
  unsigned char name_l = (unsigned char) p_dat[0];
  char username[256] = {0}, color[7] = {0};
  memcpy(username, &p_dat[1], name_l);
  memcpy(color, &p_dat[1 + name_l], 6);

  client->has_introduced = 1;
  strcpy(client->username, username);
  strcpy(client->color, color);

  /* maybe should check if username already exists */

  if (name_l != strlen(username) || strlen(color) != 6) {
    printf("[ERROR] err processing packet 0. \n");
    /* printf("username[%ld=%d] = %s, color = %s\n", strlen(username), name_l, username, color); */
    return -1;
  }
  /* printf("[OK] Process packet function assigned: username = %s, color = %s\n", username, color); */
  /* printf("[OK] Client username = %s, client color = %s (cstruct)\n", client->username, client->color); */

  /* sending packet 1 */
  unsigned char p[MAX_PACKET_SIZE];
  unsigned char p_id = (unsigned char) client->ID;
  unsigned char g_id = current_game->g_id;

  int packet_size = _create_packet_1(p, g_id, p_id, INIT_SIZE, MAX_X, MAX_Y, TIME_LIM, INIT_LIVES);

  if (send_prepared_packet(p, packet_size, client->socket) < 0) {
    /* printf("[ERROR] Packet 1 could not be sent.\n"); */
      return -1;
  } else {
    /* printf("[SEND PACKET 1] Success.\n"); */
  }

  return 0;
}

int process_packet_1(unsigned char* p_dat, int *client_status, game *current_game, client_struct *client) {
  *client_status = 4;
  unsigned int p_init_size = get_int_from_4bytes_lendian(&p_dat[2]); /* 4 bytes */
  unsigned int max_x = get_int_from_4bytes_lendian(&p_dat[6]); /* 4 bytes */
  unsigned int max_y = get_int_from_4bytes_lendian(&p_dat[10]); /* 4 bytes */
  unsigned int time_limit = get_int_from_4bytes_lendian(&p_dat[14]); /* 4 bytes */
  unsigned int num_of_lives = get_int_from_4bytes_lendian(&p_dat[18]); /* 4 bytes */

  current_game->g_id = p_dat[0];
  current_game->max_x = max_x;
  current_game->max_y = max_y;
  current_game->time_left = time_limit;
  current_game->time_limit = time_limit;
  current_game->initial_size = p_init_size;
  current_game->initial_size = num_of_lives;

  client->ID = p_dat[1];
  client->size = p_init_size;
  client->lives = num_of_lives;

  /* printf("[REC PACKET 1] Game ID = %d, Player ID = %d, p_init_size = %d, field size (%d, %d), time %d, lives %d\n",
          current_game->g_id, client->ID, p_init_size, max_x, max_y, time_limit, num_of_lives);
*/
  *client_status = 5;

  return 0;
}

int process_packet_2(unsigned char* p_dat, client_struct* client, game *current_game) {
  unsigned char g_id, p_id, byte3;
  int ready;
  g_id = p_dat[0];
  p_id = p_dat[1];
  byte3 = p_dat[2];
  int ready_bitNumber = 2; /* from left */
  ready = get_bit(byte3, ready_bitNumber);

  /* printf("[REC PACKET 2] g_id = %d, p_id = %d, byte 3 = %d, ready = %d\n", g_id, p_id, byte3, ready);
*/
    if (g_id != current_game->g_id || p_id != client->ID) {
        printf("There was mistake in packet2.\n");
    }

  client->ready = ready;

  return 0;
}

int process_packet_3(unsigned char* p, int* client_status, client_struct **clients, game *current_game, dot **dots) {
  unsigned char g_id;
  unsigned short int n_players, n_dots;
  unsigned int time_left;
  /* client_struct* clients[MAX_CLIENTS]; */

  /* printf("[REC PACKET 3]\n"); */

  /* if packet 3 received, set player to ready state (done by server, client should adapt - that is why this function here)... */
  if (*client_status == 5) {
    *client_status = 6;
    /* printf("Client status set to 6 from 5 although player did not get ready...\n"); */
  }

  int name_l, total_client_len = 0;

  g_id = p[0];
  n_players = get_int_from_4bytes_lendian(&p[1]); /* 2 byte int */

  current_game->clients_active = n_players;

    if (g_id != current_game->g_id) {
        printf("Error in packet 3: g_id=%d but current_game->g_id=%d\n", g_id, current_game->g_id);
    }

    int i;

  /* get PLAYER DATA */
  /* printf("\n=== PLAYERS IN GAME === \n"); */
  for(i=0; i < n_players; ++i) {
      if (!clients[i]) {
          //* malloc client ONLY if not mallocated already before */
          clients[i] = (client_struct *) malloc(sizeof(client_struct));
          if (clients[i] == NULL) {
              printf("[WARNING] Malloc did not work. Cannot create client in packet receiving.\n");
              return -1;
          }
      }

    client_struct* client = clients[i];

    client->ID = p[3 + total_client_len];
    name_l = p[4 + total_client_len];
    memcpy(client->username, &p[5 + total_client_len], name_l);
    unsigned int x = get_int_from_4bytes_lendian(&p[5 + name_l + total_client_len]); /* 4 bytes */
    unsigned int y = get_int_from_4bytes_lendian(&p[9 + name_l + total_client_len]); /* 4 bytes */
    client->x = (float) x;
    client->y = (float) y;
    memcpy(client->color, &p[13 + total_client_len + name_l], 6);
    client->size = get_int_from_4bytes_lendian(&p[19 + total_client_len + name_l]); /* 4 bytes */
    client->score = get_int_from_4bytes_lendian(&p[23 + total_client_len + name_l]); /* 4 bytes */
    client->lives = get_int_from_4bytes_lendian(&p[27 + total_client_len + name_l]); /* 4 bytes */

    /*printf("%d: %s (id=%d, #%s), x=%f, y=%f, size=%d, score=%d, lives=%d\n",
      i+1, client->username, client->ID, client->color, client->x, client->y, client->size, client->score, client->lives);
*/
      total_client_len += 30 + name_l - 3 + 1;
  }

  n_dots = get_sh_int_2bytes_lendian(&p[3 + total_client_len]); /* 2 bytes */

  /* get DOT DATA */
  current_game->active_dots = n_dots;

  for(i=0; i < n_dots; ++i) {
      if (!dots[i]) {
          /* malloc dot ONLY if not mallocated already before */
          dots[i] = (dot *) malloc(sizeof(dot));
          if (dots[i] == NULL) {
              printf("[WARNING] Malloc did not work. Cannot create dot in packet receiving.\n");
              return -1;
          }
      }

    dots[i]->x = get_int_from_4bytes_lendian(&p[3 + total_client_len + 2 + i*8]); /* 4 bytes */
    dots[i]->y = get_int_from_4bytes_lendian(&p[3 + total_client_len + 2 + i*8 + 4]); /* 4 bytes */
  }

  /* print DOT DATA */
  /*
  printf("\n=== DOTS === \n");
  for(i=0; i < n_dots; ++i) {
    if (i == n_dots - 1) printf("[%d, %d]\n", dots[i]->x, dots[i]->y);
    else printf("[%d, %d], ", dots[i]->x, dots[i]->y);
  } */

  time_left = get_int_from_4bytes_lendian(&p[3 + total_client_len + 2 + n_dots*8]); /* 4 bytes */
  current_game->time_left = time_left;

  /* set to game is running */
  if (*client_status == 6 || *client_status == 7) {
      *client_status = 8;
  }

  /* printf("\n"); */

  return 0;
}

int process_packet_4(unsigned char* p_dat, client_struct* client) {
  /* unsigned char g_id, p_id; */
  unsigned char byte3;

  char w_pos = 3, a_pos = 2, s_pos = 1, d_pos = 0; /* bitNumber, see get_bit */
  /* g_id = p_dat[0];
  p_id = p_dat[1]; */
  byte3 = p_dat[2];
  client->wasd[0] = get_bit(byte3, w_pos); /* W pressed*/
  client->wasd[1] = get_bit(byte3, a_pos); /* A pressed */
  client->wasd[2] = get_bit(byte3, s_pos); /* S pressed */
  client->wasd[3] = get_bit(byte3, d_pos); /* D pressed */

/*   printf("[REC PACKET 4] pressed(w,a,s,d)=(%d,%d,%d,%d), client=%d\n", w, a, s, d, client->ID); */

  return 0;
}

int process_packet_5(unsigned char* p_dat, int *client_status) {
  unsigned char g_id, p_id;
  g_id = p_dat[0];
  p_id = p_dat[1];

  unsigned int score, time_passed;
  score = get_int_from_4bytes_lendian(&p_dat[2]); /* 4 bytes */
  time_passed = get_int_from_4bytes_lendian(&p_dat[6]); /* 4 bytes */

  printf("[REC PACKET 5] You died. Your score: %d, time passed %d. g_id=%d, p_id=%d\n", score, time_passed, g_id, p_id);

  *client_status = 9;

  return 0;
}

int process_packet_6(unsigned char* p_dat, int* client_status) {
  unsigned char g_id, p_id;
  unsigned short int n_players;
  g_id = p_dat[0];
  p_id = p_dat[1];

  unsigned int score;
  score = get_int_from_4bytes_lendian(&p_dat[2]); /* 4 bytes */
  n_players = get_int_from_4bytes_lendian(&p_dat[6]); /* 2 bytes */

  printf("[REC PACKET 6] GAME ENDED. g_id=%d, p_id=%d, p_score=%d\n", g_id, p_id, score);

  int i;
  unsigned int bytesRead = 0;
  printf("==== LEADERBOARD =====\n");
  for (i = 0; i < n_players; i++) {
    unsigned char name_l;
    char username[256] = {0};
    unsigned int score;

    name_l = p_dat[8 + bytesRead]; /* 1 byte */
    memcpy(username, &p_dat[8 + bytesRead + 1], name_l); /* name_l bytes */
    score = get_int_from_4bytes_lendian(&p_dat[8 + bytesRead + 1 + name_l]); /* 4 bytes */

    bytesRead += 1 + name_l + 4;

    printf("%s has %d points\n", username, score);
  }

  *client_status = 10;

  return 0;
}

int process_packet_7 (unsigned char* p_dat) {
  /* unsigned char g_id; */
  unsigned char p_id;
  char message[MAX_MESSAGE_SIZE + 1] = {0};
  /* g_id = p_dat[0]; */
  p_id = p_dat[1];

  short int message_l;
  message_l = get_sh_int_2bytes_lendian(&p_dat[2]); /* 2 bytes */

  memcpy(message, &p_dat[4], message_l); /* name_l bytes */

  printf("[MESSAGE(%d)] %s\n", p_id, message);

  return 0;
}

void* process_incoming_packet(void* packet_info) {
    process_inc_pack_struct *p_info = (process_inc_pack_struct *) packet_info;
    unsigned char *p = p_info->p;
    int size_header = p_info->size_header;
    int size_data = p_info->size_data;
    client_struct *client = p_info->client;
    /* int c_socket = p_info->c_socket; */
    int *client_status = p_info->client_status;
    game *current_game = p_info->current_game;
    client_struct **clients = p_info->clients;
    /* unsigned char *p_id = p_info->p_id; */
    int is_server = p_info->is_server;
    dot **dots = p_info->dots;

  /* is_server = 1 (called function in server), is server = 0 (called function in client) */
  int size = size_header + size_data;

  int type = (int) p[0];

  int checksum_ok = 0;
  unsigned char checksum = get_checksum(p, size - 1);
  if (p[size-1] == checksum)
    checksum_ok = 1;
    /* printf("[OK] Checksums are correct.\n"); */
  else {
    printf("[WARNING] Packet number %d: checksums mismatch :(. Abandoned the packet. In packet %d but in server %d. \n", type, p[size-1], checksum);
  }

  int process_result = -1;
  if (checksum_ok)
      switch (type) {
        case 0:
          if (is_server) process_result = process_packet_0(&p[size_header], client, current_game);
          break;
        case 1:
          if (!is_server && size_data == 23) process_result = process_packet_1(&p[size_header], client_status, current_game, client);
          break;
        case 2:
          if (is_server) process_result = process_packet_2(&p[size_header], client, current_game);
          break;
        case 3:
          if (!is_server) process_result = process_packet_3(&p[size_header], client_status, clients, current_game, dots);
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
          /* print_bytes(&p[size_header], size - size_header); */
          process_result = process_packet_7(&p[size_header]);
          break;
        default:
          printf("Invalid packet type. Abandoned.\n");
          process_result = -1;
          break;
      }

  if (process_result < 0) {
    printf("[WARNING] Packet could not be processed by type function. \n");
  }

  free(packet_info);

  return NULL;
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
  unsigned char *p_id,
  pthread_t *process_packet_thread,
  game* current_game,
  client_struct **clients,
  dot **dots
  ) {

  int receive, socket = client_socket;
  unsigned char rec_byte[1];
  int packet_header_size_excl_div = 9;

  if (client != NULL && client->socket) socket = client->socket;

  if (*packet_cursor == 5) { /* position just after N_LEN */
      *current_packet_data_size = get_int_from_4bytes_lendian(&packet_in[1]);
      *packet_status = 2;
      /* printf("[OK] Got packet data size: %d\n", *current_packet_data_size); */
  }

  if (*packet_cursor == packet_header_size_excl_div + *current_packet_data_size + 1) {
    /* should end with 00, check it */
    receive = recv(socket, rec_byte, 1, 0);
    /* printf("Receiving packet's element %d: %c (%d)\n", *packet_cursor, printable_char(rec_byte[0]), rec_byte[0]); */
  	if (receive > 0) { /* received byte */
      if (rec_byte[0] == 0) { /* divisor */
        receive = recv(socket, rec_byte, 1, 0);
        /* printf("Receiving packet's element %d: %c (%d)\n", *packet_cursor, printable_char(rec_byte[0]), rec_byte[0]); */
        if (receive > 0) { /* received successfully */
          if (rec_byte[0] == 0) { /* new packet */
          } else {
            printf("[WARNING] Packet did not end with 00\n");
            return -1;
          }
        } else {
          printf("[WARNING] Errr receiving ending 00\n");
          return -1;
        }
      } else {
        printf("[WARNING] Packet did not end with 00\n");
        return -1;
      }
    } else {
      printf("[WARNING] Err receiving ending 00\n");
      return -1;
    }

    /* malloc packet_info struct */
    process_inc_pack_struct *packet_info = (process_inc_pack_struct *) malloc(sizeof(process_inc_pack_struct));
  	if (packet_info == NULL) {
  	    printf("[ERROR] Could not allocate process_inc_pack_struct structure\n");
  	    return -1;
  	}

  	packet_info->p = packet_in;
  	packet_info->size_header = packet_header_size_excl_div;
  	packet_info->size_data = *current_packet_data_size + 1;
  	packet_info->client = client;
  	packet_info->clients = clients;
  	packet_info->c_socket = socket;
  	packet_info->client_status = client_status;
  	packet_info->current_game = current_game;
  	packet_info->p_id = p_id;
  	packet_info->is_server = is_server;
  	packet_info->dots = dots;
    /* creating new thread creates many problems.. :(
     * if really want to, create it but probably need to create locks accordingly
     * perhaps mutex lock only one client message sequence or smth..
     * currently cannot process 2 simultenously incoming packets because something is not working then
     *
     * */
    if (process_packet_thread) {} /* just to avoid warning... */
    /* pthread_create(process_packet_thread, NULL, &process_incoming_packet, packet_info); */

    /* printf("[OK] Reached end of packet reading... Current cursor pos. %d\n", *packet_cursor); */
    process_incoming_packet(packet_info);
    /* process_incoming_packet(packet_in, packet_header_size_excl_div, *current_packet_data_size + 1, client, socket, client_status, g_id, p_id, is_server); */
    *packet_status = 0;
    *current_packet_data_size = 0;
    *packet_cursor = 0;
  }

  receive = recv(socket, rec_byte, 1, 0);
	if (receive > 0) { /* received byte */
        /* printf("Receiving packet's element %d: %c (%d)\n", *packet_cursor, printable_char(rec_byte[0]), rec_byte[0]); */

        if (rec_byte[0] == 0) { /* divisor */
      /* printf("Receiving packet's element %d: %c (%d)\n", *packet_cursor, printable_char(rec_byte[0]), rec_byte[0]); */
      receive = recv(socket, rec_byte, 1, 0);
      if (receive > 0) { /* received successfully */
          /* printf("Receiving packet's element %d: %c (%d)\n", *packet_cursor, printable_char(rec_byte[0]), rec_byte[0]); */
          if (rec_byte[0] == 0) { /* new packet */
          if (*packet_status > 0) {/* previous packet should have been finished => error */
            printf("[WARNING] SHOULD NOT HAPPEN.\n");
            /* continue; */
          }
          *packet_status = 1;
          *current_packet_data_size = 0;
          *packet_cursor = 0;
          /* continue; */
        } else { /* error */
          printf("[WARNING] Packet invalid. Contained 0. From socket %d. Packet dropped. \n", socket);
          return -1;
        }
      } else {
        printf("[WARNING] Could not second recv after 0. Socket %d\n", socket);
        return -1;
      }
    } else { /* not divisor */
      if (*packet_status == 0) { /* should be started => here is error */
        printf("[WARNING] Something wrong with packet [no. 2]. Socket %d\n", socket);
        printf("p_status = %d, byte = %c (%d), p_cursor = %d, p_data_Size = %d\n", *packet_status, printable_char(rec_byte[0]), rec_byte[0], *packet_cursor, *current_packet_data_size);
        return -1;
      }

      if (rec_byte[0] == 1) {
        receive = recv(socket, rec_byte, 1, 0);
        if (receive > 0) { /* received successfully */
            /* printf("Receiving packet's element %d: %c (%d)\n", *packet_cursor, printable_char(rec_byte[0]), rec_byte[0]); */
            if (rec_byte[0] == 2) { /* new packet */
              packet_in[*packet_cursor] = 0; /* 12 is escaped 0 */
              (*packet_cursor)++;
              /* continue; */
          } else if (rec_byte[0] == 3) {
            packet_in[*packet_cursor] = 1; /* 13 is escaped 1 */
            (*packet_cursor)++;
            /* continue; */
          } else { /* error */
            printf("[WARNING] Packet invalid. Contained 1 and no following 2/3. From socket %d. Packet dropped. \n", socket);
            return -1;
          }
        } else {
          printf("[WARNING] Could not second recv after 1. Socket %d\n", socket);
          return -1;
        }
      } else {
        packet_in[*packet_cursor] = rec_byte[0];
        (*packet_cursor)++;
      }
    }

  } else {
    if (client_status && (*client_status == 9 || *client_status == 10))
      return -1;
    /* printf("[WARNING] Error recv. Smth.\n"); */
    /* return -1; */
  }

    return 1; /* success */
}

int set_packet_header(unsigned char type, unsigned char* p, unsigned int N_LEN, unsigned int npk, unsigned char* xor) {
  int esc = 0;

  p[0] = 0; p[1] = 0; /* divider */
  (*xor) ^= type;
  esc += escape_assign(type, &p[2]); /* type */
  esc += process_int_lendian(N_LEN, &p[3 + esc], xor); /* N_LEN */
  esc += process_int_lendian(npk, &p[7 + esc], xor); /* npk */

  return esc;
}

int send_prepared_packet(unsigned char* p, int p_size, int socket) {
  int i;
  for (i = 0; i < p_size; i++) {
    /* printf("Sending packet's element %d: %c (%d)\n", i, printable_char(p[i]), p[i]); */
  	if (send(socket, (p + i), 1, 0) < 0) {
      printf("Error sending to socket %d\n", socket);
      return -1;
    }
  }

  return 0;
}

int process_int_lendian(int n, unsigned char* p_part, unsigned char* xor) {
  int escape_count = 0;
  unsigned char byte;

  /* byte 1 */
  byte = n & 0xFF;
  (*xor) ^= byte;
  escape_count += escape_assign(byte, &p_part[0]);
  /* byte 2 */
  byte = (n >> 8) & 0xFF;
  (*xor) ^= byte;
  escape_count += escape_assign(byte, &p_part[1 + escape_count]);
  /* byte 3 */
  byte = (n >> 16) & 0xFF;
  (*xor) ^= byte;
  escape_count += escape_assign(byte, &p_part[2 + escape_count]);
  /* byte 4 */
  byte = (n >> 24) & 0xFF;
  (*xor) ^= byte;
  escape_count += escape_assign(byte, &p_part[3 + escape_count]);

  return escape_count;
}

int process_short_int_lendian(short int n, unsigned char* p_part, unsigned char* xor) {
  int escape_count = 0;
  unsigned char byte;

  /* byte 1 */
  byte = n & 0xFF;
  (*xor) ^= byte;
  escape_count += escape_assign(byte, &p_part[0]);
  /* byte 2 */
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

double getRadius(client_struct *player) {
    return sqrt(player->size / M_PI);
}