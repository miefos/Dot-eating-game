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

#include <ncurses.h>

#define SHARED_MEMORY_SIZE 1024
/**
  Contents of shared shared_memory
  1. int gamescreen_on
     0 - off
     1 - on
  2.
**/
char* shared_memory = NULL;
int* gamescreen_on = NULL;

int get_shared_memory();


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
    return -1;
  }

  printf("[OK] Connection successful.\n");

  char message[1000];

  // fork
  if (get_shared_memory() < 0) {
    printf("[ERROR] error in get_shared_memory()\n");
    return -1;
  }


  int pid = fork();

  if (pid == 0) { // child process
    printf("[OK] Child process started.\n");
    char server_reply[1024];
    while(1) {
      // printf("Reading...\n");
      //Receive a reply from the server
      if(recv(client_socket, server_reply, 1024, 0) < 0) {
        printf("recv failed\n");
        continue;
      } else {
        // printf(" ");
        if (*gamescreen_on == 0) {
          printf("%s\n", server_reply);
        }
      }
    }
  } else {
    printf("[OK] Parent process continuing.\n");

    // sleep to prevent other notifications showing after next lines (printing prompt to enter username etc).
    usleep(1500); // 0.015s

    // enter username
    char username[51];
    printf("Please enter your username (max 50 chars): ");
    scanf("%s", username);

    // enter color (HEX)
    printf("Please enter your color (6 HEX digits): ");
    char color[7];
    scanf("%s", color);

    printf("[OK] Entered username: %s and color #%s.\n", username, color);

    // TO DO should be 1 packet sent (username + color)...

    if(send(client_socket, username, strlen(username)+1, 0) < 0) {
        printf("Send failed\n");
        return -1;
    }

    printf("[OK] Username sent to server...\n");

    if(send(client_socket, color, strlen(color)+1, 0) < 0) {
        printf("Send failed\n");
        return -1;
    }

    printf("[OK] Color sent to server...\n");

    // TO DO Message from server that everthing is ok...

    sleep(5);


    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    *gamescreen_on = 1; // should be 1 here! Changed temp.


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
  }

  endwin();


  return 0;
}



int get_shared_memory() {
  printf("[OK] Entered shared memory getter.\n");
  shared_memory = mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (shared_memory == MAP_FAILED) {
    printf("[ERORR] MAP FAILED in get_shared_memory()\n");
    return -1;
  }
  gamescreen_on = (int*) shared_memory;
  printf("[OK] Finished shared memory getter.\n");
  return 0;
}
