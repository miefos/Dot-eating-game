#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include "functions.h"


#define MAX_CLIENTS 10
#define SHARED_MEMORY_SIZE 1024

/*
  1. gcc functions.c server.c -o server
  2. start the server (./server -p=9001)
  3. open other terminal
  4. gcc functions.c client.c -o client
  5. start the client (./client -p=9001)
  6. enjoy (you can open many terminals - clients and join)
  Note. The program will count how many bytes (symbols) are sent and will display each sent symbol that many times.
  Example.
    server -p=12345
    client -a=localhost -p=12345
    > abc
    abbccc
    > XY
    XXXXYYYYY
 */

char* shared_memory = NULL;
int* client_count = NULL;
int* shared_data = NULL;

int get_shared_memory();
int gameloop();
int start_network(int port);
int process_client(int id, int socket);



int main(int argc, char** argv) {
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


  int pid = 0;
  int i;
  printf("[OK] Server started!\n");
  if (get_shared_memory() < 0) {
    printf("[ERROR] error in get_shared_memory()\n");
    return -1;
  }
  /*
    fork to have two processes - networking (child here)
    and game loop (parent here)
  */
  pid = fork();
  if (pid == 0) { // child
    if (start_network(port) < 0) { // try twice
      printf("[ERROR] error in start_network().\n");
      printf("Retrying start_network().\n");
      if (start_network(port) < 0) {
        printf("[ERROR] error retrying in start_network().\n");
        return -1;
      };
    };
  } else { // parent
    if (gameloop() < 0) {
      printf("[ERROR] error in gameloop().\n");
      return -1;
    };
  }


  return 0;
}








int get_shared_memory() {
  printf("[OK] Entered shared memory getter.\n");
  shared_memory = mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (shared_memory == MAP_FAILED) {
    printf("[ERORR] MAP FAILED in get_shared_memory()\n");
    return -1;
  }
  client_count = (int*) shared_memory;
  /* map an array like this shared_data[MAX_CLIENTS*2], incoming data first, results after */
  shared_data = (int*) (shared_memory + sizeof(int));
  printf("[OK] Finished shared memory getter.\n");
  return 0;
}


int gameloop() {
  int i = 0;
  printf("[OK] Entered the gameloop.\n");
  while (1) {
    for (i = 0; i < *client_count; i++) {
      shared_data[MAX_CLIENTS+i] += shared_data[i];
      shared_data[i] = 0;
    }
    sleep(0.1);
  }

  printf("[OK] Finished the gameloop.\n");
  return 0;
}


int start_network(int port) {
  printf("[OK] Entered the start network.\n");

  int main_socket;
  struct sockaddr_in server_address;
  int client_socket;
  struct sockaddr_in client_address;
  int client_address_size = sizeof(client_address);

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

  while (1) {
      int new_client_id = 0;
      int cpid = 0;
      /* waiting for clients */
      client_socket = accept(main_socket, (struct sockaddr*) &client_address, &client_address_size);
      if (client_socket < 0) {
          /* if client connection fails, we can still accept other connections*/
          printf("[WARNING] Error accepting client.\n");
          continue;
      }

      new_client_id = *client_count;
      *client_count += 1;
      cpid = fork();

      if (cpid == 0) {
        /* start child connection processing */
        close(main_socket);
        cpid = fork();
        if (cpid == 0) {
          if (process_client(new_client_id, client_socket) < 0) {
            printf("[ERROR] error in process_client. (errcode = %d)\n", 2220);
          };
          exit(0);
        } else {
          /* orphaning */
          wait(NULL);
          printf("[OK] Successfully orphaned client %d\n", new_client_id);
          exit(0);
        }
      } else {
        close(client_socket);
      }
  }

  printf("[OK] Finished the start network.\n");
  return 0;
}


int process_client(int id, int socket){
  int i = 0;
  char in[1];
  char out[1000];
  printf("[OK] Processing client id=%d, socket=%d \n", id, socket);
  printf("[OK] Client count %d\n", *client_count);

  int N = 0;
  while (1) {
      if( recv(socket, in , 1 , 0) < 0) {
        printf("[ERROR] recv failed\n");
        continue;
      }
      if (in[0] != '\n' && in[0] != '\r' && in[0] > 0) {
        int k = 0;
        while (k < shared_data[MAX_CLIENTS+id]+N+1) {
          out[k] = in[0];
          k++;
        }
        out[k] = '\0';
        // char server_message[] = "Hello there. IAM SERVEVR!\n";
        send(socket, out, strlen(out), 0);
        // write(socket,out,strlen(out));
        // printf("[OK] Client %d sent %c\n", id, in[0]);
        shared_data[id] = N+1;
        N++;
      } else if (in[0] == '\r'){ // carriage return (finished string)
        send(socket,"\n",sizeof(char), 0);
        N = 0;
      }
  }
};
