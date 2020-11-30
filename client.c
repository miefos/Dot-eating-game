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
#include "util_functions.h"
#include "setup.h"

int leave_flag = 0;
// client status information...
// 0 - initial
// 1 - received approval from server (1st packet)
// 2 - after 1st packet waiting for user input in send_loop to set ready status
// 3 - got input, ready status set to 3 by send_loop
// 4 - ready status sent to server (2rd packet sent)
// 5 - game lost(died) (received packet 5)
// 6 - Game ended (received packet 6)
// 7 - CURRENTLY NOWHERE CAN PUT TO 7
int client_status = 0;
unsigned char g_id = 0, p_id = 0;

void set_leave_flag() {
    leave_flag = 1;
}

void* send_loop(void* arg) {
	int* client_socket = (int *) arg;
  char message[1];

  while(1) {
    if ((message[0] =  fgetc(stdin)) != '\n') { // do not send newline
      if (client_status == 2) client_status = 3; // set client_status to 3
      else if (client_status == 0) send(*client_socket, message, 1, 0); // just send
      else if (client_status == 1) send(*client_socket, message, 1, 0); // just send
      else if (client_status == 4) { // getchar for packet 4 updates
        unsigned char p[MAX_PACKET_SIZE];

        int p_size1 =  _create_packet_7(p, g_id, p_id, "I decided to send you an update about my keypresses.");
        if (send_prepared_packet(p, p_size1, *client_socket) < 0) {
          printf("[ERROR] Packet 7 could not be sent.\n");
        } else {
          printf("[OK] Packet 7 sent successfully.\n");
        }

        char w = 0, a = 0, s = 0, d = 0;
        // somehow should determine which keys pressed ... currently hard-coded.
        // perhaps also some logic should be changed so that this not come only after fgetc
        w = 1; a = 0; s = 1; d = 1; // 1 - pressed, 0 - not pressed
        int p_size = _create_packet_4(p, &g_id, &p_id, w, a, s, d);
        if (send_prepared_packet(p, p_size, *client_socket) < 0) {
          printf("[ERROR] Packet 4 could not be sent.\n");
        } else {
          printf("[OK] Packet 4 sent successfully.\n");
        }
      }
    }
  }

  return NULL;
}

void* receive_loop(void* arg) {

  unsigned char packet_in[MAX_PACKET_SIZE];
  int result;
  // 0 = no packet, 1 = packet started, 2 = packet started, have size
  int packet_status = 0;
  int packet_cursor = 0; // keeps track which packet_in index is set last
  int packet_data_size = 0;

  int* client_socket = (int *) arg;

	while(1){
    if (leave_flag) break;
    result = recv_byte(packet_in, &packet_cursor, &packet_data_size, &packet_status, 0, NULL, *client_socket, &client_status, &g_id, &p_id);
    if (client_status == 5 || client_status == 6) { set_leave_flag(); continue; } // died :(

    if (result > 0) {
        // everything done in recv_byte already...
		} else if (result < 0){ // disconnection or error
      printf("[WARNING] Could not receive package.\n");
      set_leave_flag();
		} else { // receive == 0
      printf("Recv failed. Leave flag set.\n");
      set_leave_flag();
    }

	}

  return NULL;
}

int main(int argc, char **argv){
  unsigned char p[MAX_PACKET_SIZE];
  // unsigned char data[MAX_PACKET_SIZE];

  // catch SIGINT (Interruption, e.g., ctrl+c)
	signal(SIGINT, set_leave_flag);

  // client setup
  int port, c_socket; char ip[100];
  if ((c_socket = client_setup(argc, argv, &port, ip)) < 0) return -1;


  // get username, color
  char username[256], color[7];
  get_username_color(username, color);

	// Send 0th packet
  int packet_size = _create_packet_0(p, username, color);
  send_prepared_packet(p, packet_size, c_socket);

  // start send thread
	pthread_t send_thread;
  if(pthread_create(&send_thread, NULL, (void *) send_loop, &c_socket) != 0){
		printf("[ERROR] thread creating err. \n");
    return -1;
	}

  // start receive thread
	pthread_t receive_thread;
  if(pthread_create(&receive_thread, NULL, (void *) receive_loop, &c_socket) != 0){
		printf("ERROR: thread creating err. \n");
		return -1;
	}

  // each sec check if should finish program
	while (1){
    if(leave_flag){
      printf("The game has ended.\n");
      break;
    }
    sleep(1);
	}

  // close conn
	close(c_socket);

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
