#include "functions.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef MC_MAX_SIZE_STRING
#define MC_MAX_SIZE_STRING 256
#endif

int err_mess_return(char *message) {
  printf("[ERROR] %s\n", message);
  return -1;
}

int server_setup(int argc, char **argv, int *port) {
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

    //printf("HAPPY! %d, %s, j=%d\n", i, argv[i], j);

    if (argv[i][j] != '=') {
      continue; // not found
    }


    //printf("HAPPY2! %d, %s, j=%d\n", i, argv[i], j);

    if ((strlen(argv[i]) < strlen(key)+3) || strlen(argv[i]) - strlen(key) - 2 < 1) {
      return -3; // not found
    }


    //printf("HAPPY3! %d, %s, j=%d\n", i, argv[i], j);

    //return 0;

    //printf("HAPPY 2! %d, %s\n", i, argv[i]);

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
