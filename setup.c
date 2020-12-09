#include "functions.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h> /* toupper, isprint */
#include <inttypes.h>
#include <unistd.h>
#include "util_functions.h"
#include "setup.h"

int client_setup(int argc, char **argv, int *port, char *ip) {
  /* GET ARGS */
  /* validate parameters */
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


  /* SETUP NETWORK */
  int client_socket = socket(AF_INET, SOCK_STREAM, 0);

  /* specify an address for the socket */
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(*port);
  server_address.sin_addr.s_addr = inet_addr(ip);

  int connection_status = connect(client_socket, (struct sockaddr *) &server_address, sizeof(server_address));
  /* check for error with the connection */
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
    /* validate parameters */
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
      return -1; /* end */

    if (strlen(key) + 2 > strlen(argv[i]))
      continue;

    /* check key */
    if (argv[i][0] != '-') {
      continue; /* not found */
    }

    int j = 1;
    int breaked = 0;
    while (key[j-1]) {
      if (key[j-1] != argv[i][j]) {
        breaked = 1;
        break; /* not found */
      }
      j++;
    }

    if (breaked) {
      continue; /* not found */
    }

    if (argv[i][j] != '=') {
      continue; /* not found */
    }

    if ((strlen(argv[i]) < strlen(key)+3) || strlen(argv[i]) - strlen(key) - 2 < 1) {
      return -3; /* not found */
    }

    /* copy */
    *result = (char*) malloc(sizeof(char)*(strlen(argv[i]) - strlen(key) - 1));
    if (*result == NULL) {
      return -4; /* err malloc */
    }

    strcpy(*result, (argv[i] + strlen(key)+2));
    return strlen(*result); /* found, return strlen */

  }

  return -1; /* not found */
}

int get_port(char* key, int argc, char** argv) {
  char* port_c = NULL;
  int result = get_named_argument(key, argc, argv, &port_c);
  int i = 0;

  if (result < 0) {
    return -1; /* err */
  }

  while(port_c[i]) {
    /* printf("Currently checking %c\n", port_c[i]); */
    if (port_c[i] < 48 || port_c[i] > 57) {
      /* printf("Not only nums in port provided\n"); */
      return -2; /* err contains not numbers */
    }
    i++;
  }

  int port = atoi(port_c);
  free(port_c);

  return port;

}
