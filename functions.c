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

#ifndef MC_MAX_SIZE_STRING
#define MC_MAX_SIZE_STRING 256
#endif


int escape_assign(int num, unsigned char* packet) {
  if (num == 0) {
    packet[0] = 1;
    packet[1] = 2;
    return 1;

  } else if (num == 1) {
    packet[0] = 1;
    packet[1] = 3;
    return 1;

  } else {
    packet[0] = num & 0xFF; // cuts to one byte
  }

  return 0;
}

char printable_char(char c) {
  if (isprint(c)) return c;
  return ' ';
}

void print_bytes(void* packet, int count) {
  int i = 0;
  char *p = (char *) packet;
  if (count > 999) {
    printf("Cannot print more than 999 bytes! You asked for %d\n", count);
    return;
  }

  printf("Printing %d bytes...\n", count);
  printf("[NPK] [C] [HEX] [DEC] [ BINARY ]\n");
  printf("================================\n");
  for (i = 0; i < count; i++) {
    printf(" %3d | %c | %02x | %3d | %c%c%c%c%c%c%c%c\n", i, printable_char(p[i]), p[i], p[i],
      p[i] & 0x80 ? '1' : '0',
      p[i] & 0x40 ? '1' : '0',
      p[i] & 0x20 ? '1' : '0',
      p[i] & 0x10 ? '1' : '0',
      p[i] & 0x08 ? '1' : '0',
      p[i] & 0x04 ? '1' : '0',
      p[i] & 0x02 ? '1' : '0',
      p[i] & 0x01 ? '1' : '0'
    );
  }

}

int assign_int_to_bytes_lendian_escape(unsigned char* packet_part, int n, int should_escape) {
  int escape_count = 0;
  if (should_escape) {
    unsigned char byte;

    // byte 1
    byte = n & 0xFF;
    escape_count += escape_assign((int) byte, &packet_part[0]);

    // byte 2
    byte = (n >> 8) & 0xFF;
    escape_count += escape_assign((int) byte, &packet_part[1 + escape_count]);


    // byte 3
    byte = (n >> 16) & 0xFF;
    escape_count += escape_assign((int) byte, &packet_part[2 + escape_count]);


    // byte 4
    byte = (n >> 16) & 0xFF;
    escape_count += escape_assign((int) byte, &packet_part[3 + escape_count]);
  } else {
    packet_part[0] = n & 0xFF;
    packet_part[1] = (n >> 8) & 0xFF;
    packet_part[2] = (n >> 16) & 0xFF;
    packet_part[3] = (n >> 24) & 0xFF;
  }

  return escape_count;
}

int get_int_from_4bytes_lendian(unsigned char* the4bytes) {
  // It works only for little endian...
  return *((int *) the4bytes);
}


unsigned char get_checksum(unsigned char* packet_header, int length_header_excl_div, unsigned char* packet_data, int data_length) {
  unsigned char chk = 0;

  for (int i = 0; i < length_header_excl_div; i++) {
    chk ^= packet_header[i];
  }

  printf(" Checksum after head %d\n", chk);

  for (int i = 0; i < data_length; i++) {
    chk ^= packet_data[i];
  }

  printf(" Checksum after he2ad %d\n", chk);

  return chk;
}

int create_packet(unsigned char* packet, int type, char* data) {
  unsigned int DATA_LENGTH = strlen(data);
  int escaped_total = 0;

  if (type > 7 || type < 0) {
    printf("[WARNING] Packet type wrong. It is %d but should be [0 ... 7]. Packet not sent.\n", type);
    return 0;
  }

  packet[0] = 0; // divider
  packet[1] = 0; // divider
  escaped_total += escape_assign(type, &packet[2]);
  // packet[3,4,5,6] = DATA_LENGTH
  escaped_total += assign_int_to_bytes_lendian_escape(&packet[3 + escaped_total], DATA_LENGTH, 1);

  // packet[7,8,9,10] = NPK (int representing which is sequence the received packet is)
  escaped_total += assign_int_to_bytes_lendian_escape(&packet[7 + escaped_total], 0, 1);

  // THEN COMES DATA ... packet[11 ... (DATA_LENGTH+11)]
  int data_starts_at = 11 + escaped_total;
  memcpy(&packet[data_starts_at], data, DATA_LENGTH);
  // CHECHKSUM! (1 byte)
  // checksum everything except dividers (first 00)
  // packet header contains type, data_length, and npk... unfortunately hard to do it..
  // could escape the whole packet after setting everything but that would need large resources...
  int packet_header_size_excl_div = 9;
  unsigned char packet_header[packet_header_size_excl_div]; // total 9 bytes should be
  // content should be
  // packet_header[0] = type
  // packet_header[1,2,3,4] = data_length
  // packet_header[5,6,7,8] = npk
  packet_header[0] = type & 0xFF;
  assign_int_to_bytes_lendian_escape(&packet_header[1], DATA_LENGTH, 0);
  assign_int_to_bytes_lendian_escape(&packet_header[5], 0, 0);

  packet[DATA_LENGTH + 11 + escaped_total] = get_checksum(packet_header, packet_header_size_excl_div, packet+data_starts_at, DATA_LENGTH);

  // print_bytes(packet, 50);


  int total_packet_size =
    2 + // divider 00
    1 + // packet type (0 - 7)
    4 + // data length
    4 + // npk
    DATA_LENGTH + // data
    1 + // checksum
    escaped_total; // total escapes

  return total_packet_size;
}



// void set_leave_flag(int *flag) {
//     (*flag) = 1;
// }

int is_little_endian_system() {
  volatile uint32_t i = 0x01234567;
  return (*((uint8_t*)(&i))) == 0x67;
  /* 1 = little, 0 = big*/
}

void remove_newline(char *str) {
      if (strlen(str) > 0 && str[strlen(str)-1] == '\n')
      	str[strlen(str)-1] = '\0';
}

int get_username_color(char* username, char* color) {
  // ======================================
  // delete/comment next line-s on deployment
  printf("[DEV] Press enter to autofill.\n");



  char username_temp[260]; // actually max 255
  char color_temp[10]; // actually max 6
  // insert username
  int username_ok = 1;
  printf("Please enter your username: ");
  fgets(username_temp, 260, stdin);
  // ======================================
  // delete/comment next line-s on deployment
  if (strcmp(username_temp, "\n\0") == 0) {
    printf("[DEV] Autofill requested. The following will be assigned.\n");
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

int contains_only_hex_digits(char* str) {
  for (int i = 0; str[i] != '\0'; i++) {
    char c = toupper(str[i]);
    if ((c < 48 || c > 57) && // decimal digit
        (c < 65 || c > 70)) { // 'A' - 'F'
        return str[i]; // invalid char
    } else {
      str[i] = c; // make uppercase
    }
  }

  return -1; // yes, contains only hex digits
}

int client_setup(int argc, char **argv, int *port, char *ip) {
  //// GET ARGS
  // validate parameters
  if (argc != 3) {
    printf("[Error] Please provide IP and port argument only (ex: -a=123.123.123.123 -p=9001).\n");
    return -1;
  }

  *port = get_port("p", argc, argv);

  if (*port < 0) {
    printf("[ERROR] Cannot get port number, errno %d\n", *port);
    return -1;
  }

  char* iptemp = NULL;
  int result = get_named_argument("a", argc, argv, &iptemp);
  strcpy(ip, iptemp);

  if (result < 0 || ip == NULL) {
    printf("[ERROR] Cannot get IP, errno %d\n", result);
    return -1;
  }

  printf("[OK] Client params successful. Port: %d, ip: %s\n", *port, ip);


  //// SETUP NETWORK
  int client_socket = socket(AF_INET, SOCK_STREAM, 0);

  // specify an address for the socket
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(*port);
  server_address.sin_addr.s_addr = inet_addr(ip);

  int connection_status = connect(client_socket, (struct sockaddr *) &server_address, sizeof(server_address));
  // check for error with the connection
  if (connection_status < 0) {
    printf("[ERROR] There was an error with the connection.\n");
    printf("============================================================\n");
    printf("client socket = %d, server_address = %p, sizeof struct = %ld\n", client_socket, (struct sockaddr *) &server_address, sizeof(server_address));
    printf("connection_status = %d, errno = %d\n", connection_status, errno);
    printf("Port = %d, ip = %s\n", *port, ip);
    return -1;
  }

  return client_socket;
}


int server_network_setup(int* main_socket, struct sockaddr_in* server_address, int port) {

    *main_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (*main_socket < 0) {
      printf("[ERROR] Cannot open main socket.\n");
      close(*main_socket);
      return -1;
    }
    printf("[OK] Main socket created.\n");

    server_address->sin_family = AF_INET;
    server_address->sin_addr.s_addr = INADDR_ANY;
    server_address->sin_port = htons(port);

    if (bind(*main_socket, (struct sockaddr*) server_address, sizeof(*server_address)) < 0) {
      printf("[ERROR] Cannot bind the main server socket.\n");
      printf("============================================================\n");
      printf("main socket = %d, server_address = %p, sizeof struct = %ld\n", *main_socket, (struct sockaddr *) server_address, sizeof(*server_address));
      printf("Port = %d, errno = %d\n", port, errno);
      close(*main_socket);
      return -1;
    }
    printf("[OK] Main socket binded.\n");

    if (listen(*main_socket, MAX_CLIENTS) < 0) {
      printf("[ERORR] Error listening to main socket.\n");
      close(*main_socket);
      return -1;
    }
    printf("[OK] Main socket is listening\n");

    return 0;
}



int server_parse_args(int argc, char **argv, int *port) {
    // validate parameters
    if (argc != 2) {
      printf("[Error] Please provide port argument only (ex: -p=9001).\n");
      return -1;
    }

    *port = get_port("p", argc, argv);

    if (*port < 0) {
      printf("[ERROR] Cannot get port number, %d\n", *port);
      return -1;
    }

    printf("[OK] Server starting on port %d.\n", *port);

    return 0;
}

int get_named_argument(char* key, int argc, char **argv, char** result) {
  int i;
  for (i = 0; i < argc; i++) {
    if (!strcmp(argv[i], "--"))
      return -1; // end

    if (strlen(key) + 2 > strlen(argv[i]))
      continue;

    // check key
    if (argv[i][0] != '-') {
      continue; // not found
    }

    int j = 1;
    int breaked = 0;
    while (key[j-1]) {
      if (key[j-1] != argv[i][j]) {
        breaked = 1;
        break; // not found
      }
      j++;
    }

    if (breaked) {
      continue; // not found
    }

    if (argv[i][j] != '=') {
      continue; // not found
    }

    if ((strlen(argv[i]) < strlen(key)+3) || strlen(argv[i]) - strlen(key) - 2 < 1) {
      return -3; // not found
    }

    // copy
    *result = (char*) malloc(sizeof(char)*(strlen(argv[i]) - strlen(key) - 1));
    if (*result == NULL) {
      return -4; // err malloc
    }

    strcpy(*result, (argv[i] + strlen(key)+2));
    return strlen(*result); // found, return strlen

  }

  return -1; // not found
}


int get_port(char* key, int argc, char** argv) {
  char* port_c = NULL;
  int result = get_named_argument(key, argc, argv, &port_c);
  int i = 0;

  if (result < 0) {
    return -1; // err
  }

  while(port_c[i]) {
    /* printf("Currently checking %c\n", port_c[i]); */
    if (port_c[i] < 48 || port_c[i] > 57) {
      /* printf("Not only nums in port provided\n"); */
      return -2; // err contains not numbers
    }
    i++;
  }

  int port = atoi(port_c);
  free(port_c);

  return port;

}



// int memcpy_and_escape(void* destination, void* source, int size) {
//   /* non-machine-compatible (results can differ due to endianness)*/
//   printf("===========memcpy===========\n");
//   int i = 0, num_replaced = 0;
//   char* d = destination;
//   char* s = source;
//   for (i = 0; i < size; i++) {
//     if (s[i] == 0) {
//       d[i + num_replaced] = 1;
//       d[i + 1 + num_replaced] = 2;
//       num_replaced++;
//       printf("found 0: %c (%d)\n", s[i], s[i]);
//     } else if (s[i] == 1){
//       d[i + num_replaced] = 1;
//       d[i + 1 + num_replaced] = 3;
//       num_replaced++;
//       printf("found 1: %c (%d)\n", s[i], s[i]);
//       printf("d = %s, s = %s\n", d, s);
//     } else {
//       d[i + num_replaced] = s[i];
//     }
//   }
//   printf("===========memcpy===========\n");
//
//   return num_replaced;
// }

