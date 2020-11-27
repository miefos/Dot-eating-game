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

int leave_flag = 0;

void set_leave_flag() {
    leave_flag = 1;
}

void set_data_packet_0 (char* data, char* username, char* color, int name_len) {
  // Data packet contents:
  // ============================
  // [byte 0] = chars in username
  // [byte 1 ... strlen(username) + 1] = username
  // [byte strlen(username) + 1 ... strlen(username) + 1 + 6] = color
  // ============================
  // note for color bytes: there are 6 chars in color - 6 hex digits.

  if (name_len == 0) {
    printf("[ERROR] This should not happen so no further analysis.\n");
  }

  data[0] = name_len & 0xFF; // cuts to 1 byte
  memcpy(data + 1, username, name_len); // no need to escape since its string
  memcpy(data + 1 + name_len, color, 6); // no need to escape since its string
  // null terminator, will NOT be in packet but helpful in creating packet...
  data[1 + name_len + 6] = '\0';
}

void* send_loop(void* arg) {
	int* client_socket = (int *) arg;
  char message[1];

  while(1) {
    message[0] = fgetc(stdin);

    // if (strcmp(message, "quit") == 0) {
    // 	set_leave_flag();
    // 	return NULL;
    // } else {
      if (message[0] != '\n') // do not send newline
        send(*client_socket, message, 1, 0);
    // }

  }

  return NULL;
}

void* receive_loop(void* arg) {
  int* client_socket = (int *) arg;
	char message[2] = {'\0'};
  while (1) {
    if (recv(*client_socket, message, 1, 0) > 0) {
      // if (strcmp(message, "quitfs") == 0) { // quit from server
      //   printf("Received quitfs\n");
      //   set_leave_flag();
      //   return NULL;
      // }
      printf("%s", message);
    } else break; // disconnection or error
  }

  return NULL;
}





int main(int argc, char **argv){
  unsigned char packet[MAX_PACKET_SIZE];
  char data[MAX_PACKET_SIZE];

  // catch SIGINT (Interruption, e.g., ctrl+c)
	signal(SIGINT, set_leave_flag);

  // client setup
  int port, client_socket; char ip[100];
  if ((client_socket = client_setup(argc, argv, &port, ip)) < 0) return -1;

  // get username, color
  char username[256], color[7];
  get_username_color(username, color);

	// Send 0th packet
  set_data_packet_0(data, username, color, strlen(username));
  int packet_size = create_packet(packet, 0, data);

  for (int i = 0; i < packet_size; i++) {
    printf("Sending 0th packet's element %d: %c (%d)\n", i, printable_char(packet[i]), packet[i]);
  	send(client_socket, (packet + i), 1, 0);
  }

  if (packet_size > 0) {
    printf("0th packet sent.\n");
  } else {
    printf("[ERROR] 0th packet not sent\n");
    return -1;
  }


	pthread_t send_thread;
  if(pthread_create(&send_thread, NULL, (void *) send_loop, &client_socket) != 0){
		printf("[ERROR] thread creating err. \n");
    return -1;
	}

	// char username[270];
  // char ;
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
for shared memory could not use char str[200] = ""; needed to use strcpy... Because changed address...
problem related unsigned char

    **/

