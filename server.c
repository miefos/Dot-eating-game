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

#define MAX_CLIENTS 10
#define BUFFER_SIZE 2048

typedef struct {
  int socket;
  int ID;
  char username[256];
  char color[7];
} client_struct;


int client_count = 0;
int* ID = NULL;
client_struct* clients[MAX_CLIENTS]; // creates array of clients with size MAX_CLIENTS
int leave_flag = 0;

void* gameloop(void* arg);
int start_network(int port);
void* process_client(void* arg);

void send_packet(char *packet); // send packet to all clients
void broadcast_packet(char *packet, int id); // do not send to specified ID
void remove_client(int id);
void add_client(client_struct* client);

void set_leave_flag() {
    leave_flag = 1;
}

/**
======================
Main - OK
======================
**/
int main(int argc, char **argv){

  // validate parameters
  if (argc != 2) {
    printf("[Error] Please provide port argument only (ex: -p=9001).\n");
    return -1;
  }

  int port = get_port("p", argc, argv);

  if (port < 0) {
    printf("[ERROR] Cannot get port number, %d\n", port);
    return -1;
  }

  printf("[OK] Server starting on port %d.\n", port);

  // catch SIGINT (Interruption, e.g., ctrl+c)
	// signal(SIGINT, set_leave_flag);
  
  pthread_t gameloop_thread;

	pthread_create(&gameloop_thread, NULL, &gameloop, NULL);

  if (start_network(port) < 0) {
    printf("[ERROR] error in start_network().\n");
  };


	return 0;
}



/* This function should be used when creating new threads for new clients... */
void* process_client(void* arg){
	char buffer[BUFFER_SIZE];
	char username[270];
  int specific_client_leave_flag = 0;

	client_struct* client = (client_struct *) arg;

	if(recv(client->socket, username, 270, 0) <= 0){
		printf("[ERROR] Cannot get intro packet [packet 0].\n");
		specific_client_leave_flag = 1;
	} else {
		strcpy(client->username, username);
		sprintf(buffer, "%s joined\n", client->username);
    fflush(stdout);
		printf("%s", buffer);
		broadcast_packet(buffer, client->ID);
	}

	bzero(buffer, BUFFER_SIZE);

	while(1){
    if (specific_client_leave_flag)
      break;

		int receive = recv(client->socket, buffer, BUFFER_SIZE, 0);
		if (receive > 0){
			if(strlen(buffer) > 0){
				broadcast_packet(buffer, client->ID);
				printf("%s [from %s]\n", buffer, client->username);
			}
		} else if (receive == 0 || strcmp(buffer, "quit") == 0){
			sprintf(buffer, "%s left\n", client->username);
			printf("%s", buffer);
			broadcast_packet(buffer, client->ID);
      specific_client_leave_flag = 1;
		} else {
			printf("[ERROR] recv failed \n");
			specific_client_leave_flag = 1;
		}

		bzero(buffer, BUFFER_SIZE);
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
int start_network(int port) {
  printf("[OK] Entered the start network.\n");

  int main_socket;
  struct sockaddr_in server_address;
  int client_socket;
  struct sockaddr_in client_address;
  unsigned int client_address_size = sizeof(client_address);

  main_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (main_socket < 0) {
    printf("[ERROR] Cannot open main socket.\n");
    close(main_socket);
    return -1;
  }
  printf("[OK] Main socket created.\n");

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(port);


  if (bind(main_socket, (struct sockaddr*) &server_address, sizeof(server_address)) < 0) {
    printf("[ERROR] Cannot bind the main server socket.\n");
    close(main_socket);
    return -1;
  }
  printf("[OK] Main socket binded.\n");

  if (listen(main_socket, MAX_CLIENTS) < 0) {
    printf("[ERORR] Error listening to main socket.\n");
    close(main_socket);
    return -1;
  }
  printf("[OK] Main socket is listening\n");

  pthread_t new_client_threads;

  while (1) {
    /* waiting for clients */
    client_socket = accept(main_socket, (struct sockaddr*) &client_address, &client_address_size);
    if (client_socket < 0) {
        /* if client connection fails, we can still accept other connections*/
        printf("[WARNING] Error accepting client.\n");
        continue;
    }

  	/* Check if max clients is reached */
  	if(client_count + 1 == MAX_CLIENTS){
  		printf("Max clients reached. Connection rejected.\n");
  		close(client_socket);
  		continue;
  	}

  	/* Client settings */
  	client_struct* client = (client_struct *) malloc(sizeof(client_struct));
  	client->socket = client_socket;
  	client->ID = (*ID)++;

  	/* Add client and make new thread */
  	add_client(client);
  	pthread_create(&new_client_threads, NULL, &process_client, (void *) client);

  	/* Check for connections only once 1s */
  	sleep(1);
  }

  printf("[OK] Finished the start network.\n");

  return 0;

}

/**
======================
Gameloop - TODO
======================
**/
void* gameloop(void * arg) {
  printf("[OK] Entered the gameloop.\n");
  while (1) {
    printf("Gameloop is here...\n");
    if (leave_flag) {
      break;
    }
    sleep(5);
  }

  printf("[OK] Finished the gameloop.\n");

  return NULL;
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
void add_client(client_struct* client) {
	pthread_mutex_lock(&lock1);

	for(int i=0; i < MAX_CLIENTS; ++i) {
		if(!clients[i]) {
			clients[i] = client;
			break;
		}
	}

	client_count++;

	pthread_mutex_unlock(&lock1);
}

/* Remove clients */
void remove_client(int id) {
	pthread_mutex_lock(&lock1);

	for(int i=0; i < MAX_CLIENTS; ++i) {
		if(clients[i]) {
			if(clients[i]->ID == id) {
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
