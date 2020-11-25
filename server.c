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
#define SHARED_MEMORY_SIZE 16384
#define BUFFER_SIZE 2048

typedef struct {
  int socket;
  int active;
  int ID;
  char username[256];
  char color[7];
} client_struct;


char* shared_memory = NULL;
int* client_count = NULL;
int* ID = NULL;
client_struct* clients;
int leave_flag = 0;

int get_shared_memory();
int gameloop();
int start_network(int port);
void* process_client(void* arg);

void send_packet(char *packet); // send packet to all clients
void broadcast_packet(char *packet, int id); // do not send to specified ID
void remove_client(int id);
client_struct* add_client(int client_socket);

/**
======================
TODO should send end signal to clients when ctrl+c
======================
**/
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
	signal(SIGINT, set_leave_flag);

	if (get_shared_memory() < 0) {
	    printf("[ERROR] error in get_shared_memory()\n");
	    return -1;
	}

  /*
    fork to have two processes - networking (child here)
    and game loop (parent here)
  */
  int pid = fork();
  if (pid == 0) { // child
    if (start_network(port) < 0) {
      printf("[ERROR] error in start_network().\n");
    };
  } else { // parent
    if (gameloop() < 0) {
      printf("[ERROR] error in gameloop().\n");
      return -1;
    };
    kill(pid, SIGKILL);
    wait(NULL);
    sleep(1);
    char quitfsmes[] = "quitfs\0";
    printf("Sending quitfs\n");
    send_packet(quitfsmes);
    sleep(2);
  }

	return 0;
}



/* This function should be used when creating new threads for new clients... */
void* process_client(void* arg){
	char buffer[BUFFER_SIZE];
	char username[270];
  int specific_client_leave_flag = 0;

	(*client_count)++;
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
  (*client_count)--;
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

  pthread_t tid;

  while (1) {
    /* waiting for clients */
    client_socket = accept(main_socket, (struct sockaddr*) &client_address, &client_address_size);
    if (client_socket < 0) {
        /* if client connection fails, we can still accept other connections*/
        printf("[WARNING] Error accepting client.\n");
        continue;
    }

  	/* Check if max clients is reached */
  	if(((*client_count) + 1) == MAX_CLIENTS){
  		printf("Max clients reached. Further connections will be rejected.\n");
  		close(client_socket);
  		continue;
  	}

  	/* Add client and make new thread */
  	client_struct* new_client = add_client(client_socket);
  	pthread_create(&tid, NULL, &process_client, (void *) new_client);

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
int gameloop() {
  printf("[OK] Entered the gameloop.\n");
  while (1) {
    if (leave_flag) {
      break;
    }
    sleep(5);
  }

  printf("[OK] Finished the gameloop.\n");
  return 0;
}




/**
======================
Shared memory - OK
======================
**/
int get_shared_memory() {
  printf("[OK] Entered shared memory getter.\n");
  shared_memory = mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (shared_memory == MAP_FAILED) {
    printf("[ERORR] MAP FAILED in get_shared_memory()\n");
    return -1;
  }

  /* Set variables */
  client_count = (int *) shared_memory;
  (*client_count) = 0;

  ID = (int *) (shared_memory + sizeof(int));
  (*ID) = 0;

  clients = (client_struct* ) (shared_memory + sizeof(int) * 2);

  printf("[OK] Finished shared memory getter.\n");
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
	pthread_mutex_lock(&lock1);

  client_struct* client_ptr;

	for(int i=0; i < MAX_CLIENTS; ++i) {
		if(!(clients[i].active)) {
      clients[i].socket = client_socket;
      client_ptr = (clients + i);
			clients[i].ID = (*ID)++; // assign, then increment
      clients[i].active = 1;
			break;
		}
	}

	pthread_mutex_unlock(&lock1);

  return client_ptr;
}

/* Remove clients */
void remove_client(int id) {
	pthread_mutex_lock(&lock1);

	for(int i=0; i < MAX_CLIENTS; ++i) {
		if(clients[i].active) {
			if(clients[i].ID == id) {
        clients[i].ID = -1;
        clients[i].socket = -1;
        bzero(clients[i].username, strlen(clients[i].username));
        bzero(clients[i].color, strlen(clients[i].color));
				clients[i].active = 0;
				break;
			}
		}
	}

	pthread_mutex_unlock(&lock1);
}

/* Send a packet to all clients */
void send_packet(char *packet) {
	pthread_mutex_lock(&lock1);

	for(int i=0; i < MAX_CLIENTS; ++i) {
    if(clients[i].active) {
      if(write(clients[i].socket, packet, strlen(packet)) < 0) {
        printf("[WARNING] Could not send packet to someone.\n");
        printf("Failed with Pid=%ld\n", (long) getpid());
        printf("Socket = %d, i = %d, username %s \n", clients[i].socket, i, clients[i].username);
        printf("packet = \"%s\" \n", packet);
        // break;
      }
    }
	}

	pthread_mutex_unlock(&lock1);
}


void broadcast_packet(char *packet, int id) {
	pthread_mutex_lock(&lock1);

	for(int i=0; i < MAX_CLIENTS; ++i) {
		if(clients[i].active) {
			if (clients[i].ID != id) {
				if(write(clients[i].socket, packet, strlen(packet)) < 0) {
					printf("[ERROR] Could not send packet to someone.\n");
					break;
				}
        printf("Success with Pid=%ld\n", (long) getpid());
			}
		}
	}

	pthread_mutex_unlock(&lock1);
}
