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
  unsigned char type = 0, npk = 0;
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
  unsigned char type = 1, npk = 0;
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
  unsigned char type = 2, npk = 0, byte3 = 0;
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


int _packet3_helper_process_dots(dot** dots, unsigned short int n_dots, unsigned char* p_part, unsigned char* xor) {
  //    ==== DOT INFORMATION ====
  //    data[0,1,2,3] = x_location - unsigned int
  //    data[4,5,6,7] = y_location - unsigned int
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
  //    ==== PLAYER DATA ====
  //    data[0] = player ID
  //    data[1] = strlen(name)
  //    data[2 ... strlen(name)] = name
  //    data[2 + strlen(name) ... 5 + strlen(name)] = player's location x (unsigned int)
  //    data[6 + strlen(name) ... 9 + strlen(name)] = player's location y (unsigned int)
  //    data[10 + strlen(name) ... 15 + strlen(name)] = color (6 hex digits)
  //    data[16 + strlen(name) ... 19 + strlen(name)] = player's size (unsigned int)
  //    data[20 + strlen(name) ... 23 + strlen(name)] = player's score (unsigned int)
  //    data[24 + strlen(name) ... 27 + strlen(name)] = player' s lives (unsigned int)
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

      bytesWritten += 27 + esc_internal + name_l + 1;

		}
	}

  *clients_total_len = bytesWritten;

  return esc_total; // used only for N_LEN
}

//    ================ [3rd PACKET DATA CONTENT] ==================
//    packet[0] = game ID
//    packet[1] = Num of players
//    P_LEN = len_of_players_data*Num Of players;
//    packet[2 ... 2 + P_LEN] = PLAYER DATA*
//    packet[3 + P_LEN ... 4 + P_LEN] = Num of dots (unsigned short int), max 65536
//    packet[5 + P_LEN ... 5 + P_LEN + 8*Num of dots] = DOT INFORMATION**
//    packet[5 + P_LEN + 8*Num of dots + 1 ... 5 + P_LEN + 8*Num of dots + 4] = Time left (unsigned int)
//    =========================================================

int _create_packet_3(unsigned char* p, unsigned char g_id, client_struct** clients, unsigned short int n_dots, dot** dots, unsigned int time_left) {
  unsigned char p_user_data[MAX_PACKET_SIZE];
  const unsigned char type = 3;
  unsigned char xor = 0;
  int esc = 0, c_total_len = 0, client_count = 0;
  int esc_cl = _packet3_helper_process_clients(clients, &client_count, p_user_data, &c_total_len, &xor); // process all clients
  const unsigned char N_LEN = (c_total_len - esc_cl) + 1 + 2 + 2 + 8*n_dots + 4;

  // should be implemented real
  unsigned char npk = 0;

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

int process_packet_1(unsigned char* p_dat, int c_socket, int *client_status) {
  *client_status = 1;
  unsigned char g_id = p_dat[0];
  unsigned char p_id = p_dat[1];
  unsigned int p_init_size = get_int_from_up_to_4bytes_lendian(&p_dat[2]);
  unsigned int max_x = get_int_from_up_to_4bytes_lendian(&p_dat[6]);
  unsigned int max_y = get_int_from_up_to_4bytes_lendian(&p_dat[10]);
  unsigned int time_limit = get_int_from_up_to_4bytes_lendian(&p_dat[14]);
  unsigned int num_of_lives = get_int_from_up_to_4bytes_lendian(&p_dat[18]);

  printf("[OK] Got packet 1. Game ID = %d, Player ID = %d, p_init_size = %d, field size (%d, %d), time %d, lives %d\n",
          g_id, p_id, p_init_size, max_x, max_y, time_limit, num_of_lives);

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
  int packet_size = _create_packet_2(p, g_id, p_id, ready);

  if (send_prepared_packet(p, packet_size, c_socket) < 0) {
    printf("[ERROR] Packet 1 could not be sent.\n");
      return -1;
  } else {
    printf("[OK] Packet 1 sent successfully.\n");
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
  ready = (byte3 >> 2);

  printf("Received packet 2. g_id = %d, p_id = %d, byte 3 = %d, ready = %d\n", g_id, p_id, byte3, ready);

  client->ready = ready;
  return 0;
}

void trash_function(void* arg1, void *arg2, void* arg3, void* arg4, void* arg5) {

}


int process_packet_3(unsigned char* p, int c_socket, int* client_status) {
  unsigned char g_id;
  unsigned short int n_players, n_dots;
  unsigned int time_left;
  client_struct* clients[MAX_CLIENTS];

  char username[256] = {0}, color[7] = {0};
  int name_l, total_client_len = 0;

  printf("[OK] Packet 3 received.\n");

  g_id = p[0];
  n_players = get_int_from_up_to_4bytes_lendian(&p[1]); // 2 byte int

  // get PLAYER DATA
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
    client->x = get_int_from_up_to_4bytes_lendian(&p[5 + name_l + total_client_len]);
    client->y = get_int_from_up_to_4bytes_lendian(&p[9 + name_l + total_client_len]);
    memcpy(client->color, &p[13 + total_client_len + name_l], 6);
    client->size = get_int_from_up_to_4bytes_lendian(&p[19 + total_client_len + name_l]);
    client->score = get_int_from_up_to_4bytes_lendian(&p[23 + total_client_len + name_l]);
    client->lives = get_int_from_up_to_4bytes_lendian(&p[27 + total_client_len + name_l]);

    clients[i] = client;

    printf("[OK] New client received. %s (id=%d, color=%s), x=%d, y=%d, size=%d, score=%d, lives=%d \n",
      client->username, client->ID, client->color, client->x, client->y, client->size, client->score, client->lives);

      total_client_len += 30 + name_l - 3 + 1;

  }

  n_dots = get_int_from_up_to_4bytes_lendian(&p[3 + total_client_len]);
  dot* dots[n_dots];

  // get DOT DATA
  for(int i=0; i < n_dots; ++i) {
    // malloc client
    dot* somedot = (dot *) malloc(sizeof(dot));
    if (somedot == NULL) {
      printf("[WARNING] Malloc did not work. Cannot create dot in packet receiving.\n");
      return -1;
    }

    somedot->x = get_int_from_up_to_4bytes_lendian(&p[3 + total_client_len + 2 + i*8]);
    somedot->y = get_int_from_up_to_4bytes_lendian(&p[3 + total_client_len + 2 + i*8 + 4]);

    dots[i] = somedot;

    printf("[OK] New dot received: x = %d, y = %d\n", somedot->x, somedot->y);
  }

  time_left = get_int_from_up_to_4bytes_lendian(&p[3 + total_client_len + 2 + n_dots*8]);

  printf("[OK] time left: %d", time_left);

  trash_function(dots, clients, &g_id, username, color); // used to disable warnings not useful

  return 0;
}

void process_incoming_packet(unsigned char *p, int size_header, int size_data, client_struct* client, int c_socket, int *client_status, int is_server) {
  // is_server = 1 (called function in server), is server = 0 (called function in client)
  int size = size_header + size_data;

  unsigned char checksum = get_checksum(p, size - 1);
  if (p[size-1] == checksum)
    printf("[OK] Checksums are correct. From packet is %d, from server is %d\n", p[size-1], checksum);
  else {
    printf("[WARNING] Packet checksums mismatch :(. Abandoned the packet. In packet %d but in server %d. \n", p[size-1], checksum);
    return;
  }

  int type = (int) p[0];

  int process_result = -1;
  switch (type) {
    case 0:
      if (is_server) process_result = process_packet_0(&p[size_header], client);
      break;
    case 1:
      if (!is_server && size_data == 23) process_result = process_packet_1(&p[size_header], c_socket, client_status);
      break;
    case 2:
      if (is_server) process_result = process_packet_2(&p[size_header], client);
      break;
    case 3:
      if (!is_server) process_result = process_packet_3(&p[size_header], c_socket, client_status);
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
  client_struct* client,
  int client_socket,
  int *client_status
  ) {

  int receive, socket = client_socket;
  unsigned char rec_byte[1];
  int packet_header_size_excl_div = 9;

  if (client != NULL) socket = client->socket;

  if (*packet_cursor == 5) { // position just after N_LEN
      *current_packet_data_size = get_int_from_up_to_4bytes_lendian(&packet_in[1]);
      *packet_status = 2;
      printf("[OK] Got packet data size: %d\n", *current_packet_data_size);
  }

  if (*packet_cursor == packet_header_size_excl_div + *current_packet_data_size + 1) {
    // printf("[OK] Reached end of packet reading... Current cursor pos. %d\n", *packet_cursor);
    process_incoming_packet(packet_in, packet_header_size_excl_div, *current_packet_data_size + 1, client, socket, client_status, is_server); // TODO make seperate thread or smth so it can continue reading packets...
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
