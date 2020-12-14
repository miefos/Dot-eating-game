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
#include <signal.h>
#include <time.h>
#include "raylib.h"
#include "functions.h"
#include "util_functions.h"
#include "setup.h"

/* still to do... */
/* packet 6 / 5 remove client */
/* SERVER SOMETIMES DO NOT ACCEPT CLIENT ... Hmm..*/
/* check that pcket 6 should contain all members and do not exclude current member... etc */

game *current_game;

int client_count = 0;
int ID = 0; /* player id */
client_struct* clients[MAX_CLIENTS];

dot* dots[INITIAL_N_DOTS];
volatile int leave_flag = 0;

int gameloop();
void* start_network(void* arg);
void* process_client(void* arg);
void set_leave_flag();
void send_packet_to_all(unsigned char *p, int p_size, int send_to_ready_only, int p_type);
void broadcast_packet(char *packet, int id); /* do not send to specified ID */
void remove_client(int id);
client_struct* add_client(int client_socket);

void create_dots() {
	int i;
	for (i = 0; i < INITIAL_N_DOTS; i++) {
		/* malloc dot */
		dot* some_dot = (dot *) malloc(sizeof(dot));
		if (some_dot == NULL) {
			printf("[WARNING] Malloc did not work. Dot not created.\n");
			return;
		}

		some_dot->x = i;
		some_dot->y = INITIAL_N_DOTS - i;
		dots[i] = some_dot;
	}
}

void free_dots() {
	int i;
	for (i = 0; i < INITIAL_N_DOTS; i++) {
		if (dots[i]) {
			free(dots[i]);
		}
	}
}

int main(int argc, char **argv) {
  /* catch CTRL+C */
	signal(SIGINT, set_leave_flag);
    /* create dots */
	create_dots();
	/* create game */
    if ((current_game = (game*) malloc(sizeof(game))) == NULL) {
        printf("[ERROR] Cannot malloc game.\n");
        return -1;
    }
    current_game->g_id = 222;
    current_game->time_left = 123;
  /* server setup */
  int port;
  if (server_parse_args(argc, argv, &port) < 0) return -1;
  /* networking_thread */
  pthread_t networking_thread;
  pthread_create(&networking_thread, NULL, &start_network, &port);
  /* start network */
  if (gameloop() < 0) printf("[ERROR] error in gameloop().\n");
  /* return */
	free_dots();
	return 0;
}

/* This function should be used when creating new threads for new clients... */
void* process_client(void* arg){
	unsigned char packet_in[MAX_PACKET_SIZE];
  int client_leave_flag = 0, result;
  /* 0 = no packet, 1 = packet started, 2 = packet started, have size */
  int packet_status = 0;
  int packet_cursor = 0; /* keeps track which packet_in index is set last */
  int current_packet_data_size = 0; /* from packet itself not counting */

    pthread_t process_packet_thread;

	client_struct* client = (client_struct *) arg;

	while(1){
    if (client_leave_flag) break;

    if ((result = recv_byte(packet_in, &packet_cursor, &current_packet_data_size, &packet_status, 1, client, -1, NULL, NULL, &process_packet_thread, current_game, NULL)) > 0) {
        /* everything done in recv_byte already... */
		} else if (result < 0){ /* disconnection or error */
      printf("[WARNING] From %s could not receive packet.\n", client->username);
      client_leave_flag = 1;
		} else { /* receive == 0 */
      printf("Recv failed. Client leave flag set.\n");
      client_leave_flag = 1;
    }

	}

	close(client->socket);
  remove_client(client->ID);
  pthread_detach(pthread_self());

	return NULL;
}

void* start_network(void* arg) {
  int* port = (int *) arg;
  printf("[OK] Entered the start network with port %d.\n", *port);

  /* server_network_setup */
  int main_socket, client_socket;
  struct sockaddr_in server_address, client_address;
  unsigned int client_address_size = sizeof(client_address);
  if (server_network_setup(&main_socket, &server_address, *port) < 0) exit(-1);

  /* infinite loop... */
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
      close(client_socket); /* if failed adding */
      continue;
    };

  	pthread_create(&new_client_threads, NULL, &process_client, (void *) client);
  }

  printf("[Hmm...] Finished the start network. Should it?\n");

  /* return 0; */

}

int gameloop() {
  printf("[OK] Entered the gameloop.\n");

	/* try packet 3 */
	unsigned char p[MAX_PACKET_SIZE];
	float time_to_sleep = 0.5; /* secs */
	unsigned int drop_player_after = 60; /* secs */
  while (1) {
		int i;
		for(i=0; i < MAX_CLIENTS; ++i) {
			if(clients[i] && clients[i]->has_introduced) {
					/* update connection time */
					clients[i]->connected_time += time_to_sleep;

					/* set ready status if time reached */
					if (!clients[i]->ready && clients[i]->connected_time > MAX_UNREADY_TIME) {
						clients[i]->ready = 1;
						int p_size1 = _create_packet_7(p, current_game->g_id, clients[i]->ID, "\n\nYou are set ready because too long were unready!!\n\n");
						if (send_prepared_packet(p, p_size1, clients[i]->socket) < 0) {
							printf("[ERROR] Packet 7 could not be sent.\n");
						} else {
							printf("[SEND PACKET 7] Success.\n");
						}
					}

				/* check die time */
				printf("%s time is %f02, die when %d >reached\n", clients[i]->username, clients[i]->connected_time, drop_player_after);
				if (clients[i]->connected_time > drop_player_after) {
					int p_size = _create_packet_5(p, current_game->g_id, clients[i]->ID, clients[i]->score, current_game->time_left);
					if (send_prepared_packet(p, p_size, clients[i]->socket) < 0) {
						printf("[ERROR] Packet 5 could not be sent.\n");
					} else {
						printf("[SEND PACKET 5] Success.\n");
					}
				}
			}
		}

		int packet_size = _create_packet_3(p, current_game->g_id, clients, INITIAL_N_DOTS, dots, TIME_LIM - 2);
		send_packet_to_all(p, packet_size, 1, 3); /* should have return val... */

		/* init leave flag with CTRL + C. So it will send packet 6. */
    if (leave_flag) {
      printf("Leave flag detected in gameloop.\n");
			int i;
			for (i=0; i < MAX_CLIENTS; ++i) {
				if(clients[i]) {
					int p_size1 =  _create_packet_7(p, current_game->g_id, clients[i]->ID, "Sorry, time to go. I stopped the server.");
					if (send_prepared_packet(p, p_size1, clients[i]->socket) < 0) {
						printf("[ERROR] Packet 7 could not be sent.\n");
					} else {
						printf("[SEND PACKET 7] Success.\n");
					}


					int p_size = _create_packet_6(p, current_game->g_id, clients, clients[i]->ID, clients[i]->score);
					if (send_prepared_packet(p, p_size, clients[i]->socket) < 0) {
						printf("[ERROR] Packet 6 could not be sent.\n");
					} else {
						printf("[SEND PACKET 6] Success.\n");
					}
				}
			}
      break;
    }

    /* printf("[OK] Gameloop update sent...\n"); */

    nsleep(1000*time_to_sleep);
  }

  /* wait for packets to be sent etc */
  sleep(1.5);

  printf("[OK] Finished the gameloop.\n");

  return 0;
}


pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;

/* Add clients */
client_struct* add_client(int client_socket) {
  /* malloc client */
  client_struct* client = (client_struct *) malloc(sizeof(client_struct));
  if (client == NULL) {
    printf("[WARNING] Malloc did not work. Denying client.\n");
    return NULL;
  }

	pthread_mutex_lock(&lock1);

	int i;
	for(i=0; i < MAX_CLIENTS; ++i) {
		if(!clients[i]) {
      printf("New client added. Socket = %d, ID = %d\n", client_socket, ID);
      client->socket = client_socket;
      client->ID = ID;
      ID++;
      client->has_introduced = 0;
        client->ready = 0;
        client->x = GetRandomValue(0, MAX_X/2);
        client->y = GetRandomValue(0, MAX_Y/2);
        client->size = INIT_SIZE;
        client->score = 0;
        client->lives = INIT_LIVES;
        client->connected_time = 0; /* init 0 secs*/
        clients[i] = client;
      break;
		}
	}

	client_count++;

	pthread_mutex_unlock(&lock1);

  return client;
}

/* Remove clients */
void remove_client(int id) {
	pthread_mutex_lock(&lock1);

	int i;
	for(i=0; i < MAX_CLIENTS; ++i) {
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
void send_packet_to_all(unsigned char *p, int p_size, int send_to_ready_only, int p_type) {
	pthread_mutex_lock(&lock1);

	int i;
	for(i=0; i < MAX_CLIENTS; ++i) {
    if(clients[i]) {
			if ((send_to_ready_only && clients[i]->ready) || !send_to_ready_only) {
				if (send_prepared_packet(p, p_size, clients[i]->socket) < 0) {
					printf("[WARNING] Packet could not be sent.....\n");
				} else {
					printf("[SEND PACKET %i] Success.\n", p_type);
				}
			} else {
				/* printf("Packet not sent to non-ready player...\n"); */
			}
    }
	}

	pthread_mutex_unlock(&lock1);
}


void set_leave_flag() {
    leave_flag = 1;
}
