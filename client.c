#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include "functions.h"

int main(int argc, char** argv) {
  // validate parameters
  if (argc != 3) {
    printf("[Error] Please provide IP and port argument only (ex: -a=123.123.123.123 -p=9001).\n");
    return -1;
  }

  int port = get_port("p", argc, argv);

  if (port < 0) {
    printf("[ERROR] Cannot get port number, errno %d\n", port);
    return -1;
  }

  char* ip = NULL;
  int result = get_named_argument("a", argc, argv, &ip);

  if (result < 0) {
    printf("[ERROR] Cannot get IP, errno %d\n", result);
    return -1;
  }

  printf("[OK] Client with port %d and IP %s starting...\n", port, ip);


  char server_reply[2000];

  // create socket
  int client_socket;

  client_socket = socket(AF_INET, SOCK_STREAM, 0);

  // specify an address for the socket
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  server_address.sin_addr.s_addr = inet_addr(ip); // ATTENTION !!! inet_addr();

  // printf("ADDR set\n");

  int connection_status = connect(client_socket, (struct sockaddr *) &server_address, sizeof(server_address));
  // check for error with the connection
  if (connection_status < 0) {
    printf("[ERROR] There was an error with the connection.\n");
  }

	char message[1000];

  int pid = fork();

  if (pid == 0) { // child process
    while(1) {
      //Receive a reply from the server
      if( recv(client_socket , server_reply , 1 , 0) < 0) {
        // printf("recv failed\n");
      } else {
        // printf(" ");
        printf("%s", server_reply);
      }

    }
  } else {
    while (1) {
      bzero(message, strlen(message));
      scanf("%s" , message);
      // printf("Before strl: %d\n", strlen(message));
      message[strlen(message)] = '\r'; // this is used for server processing
      message[strlen(message)] = '\0';
      // printf("After strl: %d\n", strlen(message));

      //Send some data
      if( send(client_socket , message , strlen(message) , 0) < 0) {
        printf("Send failed\n");
        return 1;
      }

      // printf("Send success!\n");
    }
  }


  return 0;
}
