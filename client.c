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

#define BUFFER_SIZE 1024

int leave_flag = 0;

void set_leave_flag() {
    leave_flag = 1;
}

void remove_newline(char *str) {
      if (strlen(str) > 0 && str[strlen(str)-1] == '\n')
      	str[strlen(str)-1] = '\0';
}

void* send_loop(void* arg) {
	int* client_socket = (int *) arg;
  char message[BUFFER_SIZE] = {};

  while(1) {
    fgets(message, BUFFER_SIZE, stdin);
    setbuf(stdin, NULL); // clears overflow
    remove_newline(message);

    if (strcmp(message, "quit") == 0) {
    	set_leave_flag();
    	return NULL;
    } else {
      send(*client_socket, message, strlen(message), 0);
    }

    bzero(message, BUFFER_SIZE);
  }

  return NULL;
}

void* receive_loop(void* arg) {
  int* client_socket = (int *) arg;
	char message[BUFFER_SIZE] = {};
  while (1) {
    if (recv(*client_socket, message, BUFFER_SIZE, 0) > 0) {
      if (strcmp(message, "quitfs") == 0) { // quit from server
        printf("Received quitfs\n");
        set_leave_flag();
        return NULL;
      }
      printf("%s\n", message);
    } else { // disconnection or error
       break;
    }
	  bzero(message, BUFFER_SIZE);
  }

  return NULL;
}

int main(int argc, char **argv){

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

  if (result < 0 && ip != NULL) {
    printf("[ERROR] Cannot get IP, errno %d\n", result);
    return -1;
  }

  char username[256];
  char color[7];

	// catch SIGINT (Interruption, e.g., ctrl+c)
	signal(SIGINT, set_leave_flag);

	printf("Please enter your username: ");
  fgets(username, 256, stdin);
  remove_newline(username);
  setbuf(stdin, NULL); // clears overflow

	printf("Please enter your color (6 hex digits): ");
  fgets(color, 7, stdin);
  remove_newline(color);
  setbuf(stdin, NULL); // clears overflow

  if (strlen(username) < 1 || strlen(color) < 1) {
    printf("[ERROR] Username or color is too short. Try again.\n");
    return -1;
  }

  // create socket
  int client_socket;

  client_socket = socket(AF_INET, SOCK_STREAM, 0);

  // specify an address for the socket
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  server_address.sin_addr.s_addr = inet_addr(ip); // ATTENTION !!! inet_addr();

  int connection_status = connect(client_socket, (struct sockaddr *) &server_address, sizeof(server_address));
  // check for error with the connection
  if (connection_status < 0) {
    printf("[ERROR] There was an error with the connection.\n");
    return -1;
  }

	// Send intro
	char intro_packet[270];
	sprintf(intro_packet, "%s (#%s)", username, color);
	send(client_socket, intro_packet, strlen(intro_packet), 0);

  printf("Sending intro packet: %s\n", intro_packet);

	pthread_t send_thread;
  if(pthread_create(&send_thread, NULL, (void *) send_loop, &client_socket) != 0){
		printf("[ERROR] thread creating err. \n");
    return -1;
	}

	pthread_t receive_thread;
  if(pthread_create(&receive_thread, NULL, (void *) receive_loop, &client_socket) != 0){
		printf("ERROR: thread creating err. \n");
		return -1;
	}

	while (1){
    if(leave_flag){
      printf("The game has ended.\n");
      break;
    }
    sleep(1);
	}

	close(client_socket);

	return 0;
}



/*** Get chars...

    while (1) {
      char press_str[2];
      int ch = getch();

      // ESC - to exit
      if (ch == 27) {
        // TO DO - send disconnection
        endwin();
        printf("ESC pressed!\n");
        *gamescreen_on = 0;
        kill(pid, SIGKILL);
        wait(NULL);
        return 0;
      }

      // Arrow keys
      switch(ch) {
        case KEY_UP:
          press_str[0] = 'A';
          break;
        case KEY_DOWN:
          press_str[0] = 'B';
          break;
        case KEY_LEFT:
          press_str[0] = 'C';
          break;
        case KEY_RIGHT:
          press_str[0] = 'D';
          break;
        default:
          press_str[0] = ch;
          break;
      }

      press_str[1] = '\0';

      if(send(client_socket, press_str, sizeof(char)*strlen(press_str), 0) < 0) {
        printf("Send failed\n");
        return 1;
      }

    }
    */





    /***

Problem - client was resource intensive (took ~100% CPU). Added sleep in main while loop.


    **/
