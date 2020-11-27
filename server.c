#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "functions.h"

typedef struct {
  int socket;
  int ID;
  int has_introduced; // meaning the 0th packet is received (at first 0, when receives set it to 1)
  char username[256];
  char color[7];
} client_struct;


int client_count = 0;
int ID = 0;
client_struct* clients[MAX_CLIENTS]; // creates array of clients with size MAX_CLIENTS
volatile int leave_flag = 0;

int gameloop();
void* start_network(void* arg);
void* process_client(void* arg);

void send_packet(char *packet); // send packet to all clients
void broadcast_packet(char *packet, int id); // do not send to specified ID
void remove_client(int id);
client_struct* add_client(int client_socket);

void set_leave_flag() {
    leave_flag = 1;
}

/**
======================
Main - OK
======================
**/
int main(int argc, char **argv){

  // catch SIGINT (Interruption, e.g., ctrl+c)
	signal(SIGINT, set_leave_flag);

  // server setup
  int port;
  if (server_parse_args(argc, argv, &port) < 0) return -1;

  // networking_thread
  pthread_t networking_thread;
  pthread_create(&networking_thread, NULL, &start_network, &port);

  // start network
  if (gameloop() < 0) printf("[ERROR] error in gameloop().\n");

	return 0;
}

int process_packet_0(unsigned char* packet, int size) {
  //TODO
  return 0;
}

int process_packet_2(unsigned char* packet, int size) {
  //TODO
  return 0;
}

int process_packet_4(unsigned char* packet, int size) {
  //TODO
  return 0;
}

int process_packet_7(unsigned char* packet, int size) {
  //TODO
  return 0;
}

void process_incoming_packet(unsigned char *packet, int size) {
  printf("Welcome to packet processor!\n");
  // printf("Packet that is received here is:\n");
  // print_bytes(packet, size);

  // todo
  // 1. validate checksum
  // 2. get type
  // 3. get it to according function

  // 1.
  unsigned char checksum = get_checksum(packet, size, NULL, 0); // just skip second (I mean 3. and 4.) parameter
  if (packet[size] == checksum)
    printf("[OK] Checksums are correct. From packet is %d, from server is %d\n", packet[size], checksum);
  else {
    printf("[WARNING] Packet checksums mismatch :(. Abandoned the packet. In packet %d but in server %d. \n", packet[size], checksum);
    return;
  }

  // 2.
  int type = (int) packet[0];

  // 3.
  int process_result = -1;
  switch (type) { // only 0, 2, 4, 7 can be received by server
    case 0:
      process_result = process_packet_0(packet, size);
      break;
    case 2:
      process_result = process_packet_2(packet, size);
      break;
    case 4:
      process_result = process_packet_4(packet, size);
      break;
    case 7:
      process_result = process_packet_7(packet, size);
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

/* This function should be used when creating new threads for new clients... */
void* process_client(void* arg){
	unsigned char packet_in[MAX_PACKET_SIZE], rec_byte[1];
  int client_leave_flag = 0, receive;
  // 0 = no packet, 1 = packet started, 2 = packet started, have size
  int packet_status = 0;
  int packet_cursor = 0; // keeps track which packet_in index is set last
  // packet sizes
  int packet_size_until_data_size = 1; // only type is before
  int packet_header_size_excl_div = 10; // includes checksum 1 byte
  // int packet_size_total_without_data = packet_header_size_div + packet_header_size_excl_div + packet_chksm_size;
  int current_packet_data_size = 0; // from packet itself not counting


	client_struct* client = (client_struct *) arg;

	while(1){
    if (client_leave_flag) break;


    if (packet_cursor == packet_size_until_data_size + 4) {
        current_packet_data_size = get_int_from_4bytes_lendian(&packet_in[packet_size_until_data_size]);
        packet_status = 2;
        printf("[OK] Got packet size: %d\n", current_packet_data_size);
    }

    if (packet_cursor == packet_header_size_excl_div + current_packet_data_size) {
      printf("[OK] Reached end of packet reading... Current cursor pos. %d\n", packet_cursor);
      process_incoming_packet(packet_in, packet_header_size_excl_div + current_packet_data_size - 1); // TODO make seperate thread or smth so it can continue reading packets...
      packet_status = 0;
      current_packet_data_size = 0;
      packet_cursor = 0;
    }

    receive = recv(client->socket, rec_byte, 1, 0);
		if (receive > 0){ // received byte
      if (rec_byte[0] == 0) { // divisor
        receive = recv(client->socket, rec_byte, 1, 0);
        if (receive > 0) { // received successfully
          if (rec_byte[0] == 0) { // new packet
            if (packet_status > 0) {// previous packet should have been finished => error
              printf("[WARNING] SHOULD NOT HAPPEN.\n");
              bzero(packet_in, MAX_PACKET_SIZE);
              // process_incoming_packet(packet_in);
              continue;
            }
            packet_status = 1;
            current_packet_data_size = 0;
            packet_cursor = 0;
            bzero(packet_in, MAX_PACKET_SIZE); // This could be improved - skip 0 and then strlen
            // continue;
          } else { // error
            bzero(packet_in, MAX_PACKET_SIZE); // clean the packet
            printf("[WARNING] Packet invalid. Contained 0. From socket %d. Packet dropped. \n", client->socket);
            continue;
          }
        } else {
          printf("[WARNING] Could not second recv after 0. Socket %d\n", client->socket);
          continue;
        }
      } else { // not divisor
        if (packet_status == 0) { // should be started => here is error
          printf("[WARNING] Something wrong with packet [no. 2]. Socket %d\n", client->socket);
          bzero(packet_in, MAX_PACKET_SIZE);
          // continue;
        }
        if (rec_byte[0] == 1) {
          receive = recv(client->socket, rec_byte, 1, 0);
          if (receive > 0) { // received successfully
            if (rec_byte[0] == 2) { // new packet
                packet_in[packet_cursor] = 0; // 12 is escaped 0
                packet_cursor++;
                // continue;
            } else if (rec_byte[0] == 3) {
              packet_in[packet_cursor] = 1; // 13 is escaped 1
              packet_cursor++;
              // continue;
            } else { // error
              bzero(packet_in, MAX_PACKET_SIZE); // clean the packet
              printf("[WARNING] Packet invalid. Contained 1 and no 2/3. From socket %d. Packet dropped. \n", client->socket);
              continue;
            }
          } else {
            printf("[WARNING] Could not second recv after 1. Socket %d\n", client->socket);
            continue;
          }
        } else {
          packet_in[packet_cursor] = rec_byte[0];
          packet_cursor++;
        }
      }

      // printf("===========================\n\n\n");
      // print_bytes(packet_in, packet_cursor);

      // printf("Received %c (%d) from socket %d\n", printable_char(rec_byte[0]), rec_byte[0], client->socket);
      fflush(stdout); // to "refresh" the stdout (because no \n char and so it is not printed but kept in buffer)
      // if ()


			// if(strlen(buffer) > 0){
        // if (strcmp(buffer, "quit") == 0) { // quit
        //   sprintf(buffer, "%s left\n", client->username);
        //   printf("%s", buffer);
        //   broadcast_packet(buffer, client->ID);
        //   client_leave_flag = 1;
        // } else { // normal packet
				//   broadcast_packet(buffer, client->ID);
				//   printf("%s [from %s]\n", buffer, client->username);
        // }
			// }
		} else if (receive < 0){ // disconnection or error
      printf("[WARNING] From %s could not receive package.", client->username);
		} else { // receive == 0
      // sprintf(buffer, "%s left\n", client->username);
      // printf("%s", buffer);
      // broadcast_packet(buffer, client->ID);
      printf("Recv failed. Client leave flag set.\n");
      client_leave_flag = 1;
    }

	}

	/* stop client, thread, connection etc*/
	close(client->socket);
  remove_client(client->ID);
  free(client);
  pthread_detach(pthread_self());

	return NULL;
}


/**
======================
Start network - OK
======================
**/
void* start_network(void* arg) {
  int* port = (int *) arg;
  printf("[OK] Entered the start network with port %d.\n", *port);

  // server_network_setup
  int main_socket, client_socket;
  struct sockaddr_in server_address, client_address;
  unsigned int client_address_size = sizeof(client_address);
  if (server_network_setup(&main_socket, &server_address, *port) < 0) exit(-1);

  // infinite loop...
  pthread_t new_client_threads;
  while (1) {
    /* Check if max clients is reached */
  	if(client_count + 1 == MAX_CLIENTS){
  		printf("[WARNING] Max clients reached. Connection will be rejected.\n");
  		sleep(1);
      continue;
  	}

    /* waiting for clients */
    client_socket = accept(main_socket, (struct sockaddr*) &client_address, &client_address_size);
    if (client_socket < 0) {
        /* if client connection fails, we can still accept other connections*/
        printf("[WARNING] Error accepting client.\n");
        continue;
    }

  	/* Add client and make new thread */
    client_struct* client = add_client(client_socket);
  	if (client == NULL) {
      close(client_socket); // if failed adding
      continue;
    };

  	pthread_create(&new_client_threads, NULL, &process_client, (void *) client);
  }

  printf("[Hmm...] Finished the start network. Should it?\n");

  // return 0;

}

/**
======================
Gameloop - TODO
======================
**/
int gameloop() {
  printf("[OK] Entered the gameloop.\n");
  while (1) {
    if (leave_flag) {
      printf("Leave flag detected in gameloop.\n");
      send_packet("quitfs\0");
      break;
    }
    sleep(1);
  }

  // wait for packets to be sent etc
  sleep(1.5);

  printf("[OK] Finished the gameloop.\n");

  return 0;
}

/**
======================
Add clients - OK
Remove clients - OK
Send packet - OK
Broadcast packet - OK
======================
**/

pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;

/* Add clients */
client_struct* add_client(int client_socket) {
  // malloc client
  client_struct* client = (client_struct *) malloc(sizeof(client_struct));
  if (client == NULL) {
    printf("[WARNING] Malloc did not work. Denying client.\n");
    return NULL;
  }

	pthread_mutex_lock(&lock1);

	for(int i=0; i < MAX_CLIENTS; ++i) {
		if(!clients[i]) {
      client->socket = client_socket;
      client->ID = ID++;
      client->has_introduced = 0;
		}
	}

	client_count++;

	pthread_mutex_unlock(&lock1);

  return client;
}

/* Remove clients */
void remove_client(int id) {
	pthread_mutex_lock(&lock1);

	for(int i=0; i < MAX_CLIENTS; ++i) {
		if(clients[i]) {
			if(clients[i]->ID == id) {
        free(clients[i]);
				clients[i] = NULL;
				break;
			}
		}
	}

  client_count--;

	pthread_mutex_unlock(&lock1);
}

/* Send a packet to all clients */
void send_packet(char *packet) {
	pthread_mutex_lock(&lock1);

	for(int i=0; i < MAX_CLIENTS; ++i) {
    if(clients[i]) {
      if(write(clients[i]->socket, packet, strlen(packet)) < 0) {
        printf("[ERROR] Could not send packet to someone.\n");
        break;
      }
    }
	}

	pthread_mutex_unlock(&lock1);
}


void broadcast_packet(char *packet, int id) {
	pthread_mutex_lock(&lock1);

	for(int i=0; i < MAX_CLIENTS; ++i) {
		if(clients[i]) {
			if (clients[i]->ID != id) {
				if(write(clients[i]->socket, packet, strlen(packet)) < 0) {
					printf("[ERROR] Could not send packet to someone.\n");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&lock1);
}

