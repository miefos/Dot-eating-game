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
#include <signal.h>
#include "raylib.h" /* https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux */
#include "functions.h"
#include "util_functions.h"
#include "setup.h"

#define MAX_USERNAME_CHARS 255
#define MAX_COLOR_CHARS 6
#define MAX_CHARS_IN_TEXTBOX 15

client_struct* client;
int leave_flag = 0;
/* client status information... */
/* 0 - initial */
/* 1 - username set */
/* 2 - color set */
/* 3 - packet 0 sent */
/* 4 - received approval from server (1st packet) */
/* 5 - after 1st packet waiting for user input in send_loop to set ready status */
/* 6 - got input, ready status set to 3 by send_loop */
/* 7 - ready status sent to server (2rd packet sent) */
/* 8 - game lost(died) (received packet 5) */
/* 9 - Game ended (received packet 6) */
/* 10 - CURRENTLY NOWHERE CAN PUT TO 10 */
int client_status = 0;
unsigned char g_id = 0, p_id = 0;

void set_leave_flag() {
    leave_flag = 1;
}

void* send_loop(void* arg) {
	int* client_socket = (int *) arg;

  while(1) {
    if (client_status == 2) {
      unsigned char p[MAX_PACKET_SIZE];

      /* Send 0th packet */
      int packet_size = _create_packet_0(p, client->username, client->color);
      if (send_prepared_packet(p, packet_size, *client_socket) < 0) {
        printf("[ERROR] 0th packet could not be sent.\n");
      } else {
        printf("[OK] 0th packet sent successfully.\n");
      }
      client_status = 3;
    }
    else if (client_status == 8) { /* getchar for packet 4 updates */
      unsigned char p[MAX_PACKET_SIZE];

      /* send 7th packet */
      int p_size1 =  _create_packet_7(p, g_id, p_id, "I decided to send you an update about my keypresses.");
      if (send_prepared_packet(p, p_size1, *client_socket) < 0) {
        printf("[ERROR] Packet 7 could not be sent.\n");
      } else {
        printf("[OK] Packet 7 sent successfully.\n");
      }

      char w = 0, a = 0, s = 0, d = 0;
      /* somehow should determine which keys pressed ... currently hard-coded. */
      /* perhaps also some logic should be changed so that this not come only after fgetc */
      w = 1; a = 0; s = 1; d = 1; /* 1 - pressed, 0 - not pressed */
      int p_size = _create_packet_4(p, &g_id, &p_id, w, a, s, d);
      if (send_prepared_packet(p, p_size, *client_socket) < 0) {
        printf("[ERROR] Packet 4 could not be sent.\n");
      } else {
        printf("[OK] Packet 4 sent successfully.\n");
      }
    }
    sleep(0.1); /* send packet each 0.1s */
  }

  return NULL;
}

void* receive_loop(void* arg) {

  unsigned char packet_in[MAX_PACKET_SIZE];
  int result;
  /* 0 = no packet, 1 = packet started, 2 = packet started, have size */
  int packet_status = 0;
  int packet_cursor = 0; /* keeps track which packet_in index is set last */
  int packet_data_size = 0;

  int* client_socket = (int *) arg;

	while(1){
    if (leave_flag) break;
    result = recv_byte(packet_in, &packet_cursor, &packet_data_size, &packet_status, 0, NULL, *client_socket, &client_status, &g_id, &p_id);
    if (client_status == 8 || client_status == 9) { set_leave_flag(); continue; } /* died :( */

    if (result > 0) {
        /* everything done in recv_byte already... */
		} else if (result < 0){ /* disconnection or error */
      printf("[WARNING] Could not receive package.\n");
      set_leave_flag();
		} else { /* receive == 0 */
      printf("Recv failed. Leave flag set.\n");
      set_leave_flag();
    }

	}

  return NULL;
}

void drawUnderscoreDelMessage(int letterCount, int client_status, Rectangle textBox, char *name, char *color, int font_size, int showFromPosition, int framesCounter) {
    /* draw blinking underscore or message to del chars */
    if ((letterCount < MAX_USERNAME_CHARS && client_status == 0) || (letterCount < MAX_COLOR_CHARS && client_status == 1)) {
        int chars_length = 0;
        if (client_status == 0){
            chars_length = MeasureText(&name[showFromPosition], font_size);
        } else if (client_status == 1) {
            chars_length = MeasureText(&color[showFromPosition], font_size) + MeasureText("#", font_size);
        }

        if (((framesCounter/30)%2) == 0)
            DrawText("_", textBox.x + 8 + chars_length, textBox.y + 12, font_size, MAROON);
    }
    else if (client_status < 2) DrawText("Press BACKSPACE to delete chars...", 230, 300, 20, GRAY);
}

/* should check between packet 5 / 6 for keypress.. */

int main(int argc, char **argv){
  /*unsigned char p[MAX_PACKET_SIZE]; */
  /* unsigned char data[MAX_PACKET_SIZE]; */

  client = (client_struct*) malloc(sizeof(client_struct));
  if (client == NULL) {
    printf("[ERROR] Cannot malloc client.\n");
    return -1;
  }

  /* catch SIGINT (Interruption, e.g., ctrl+c) */
	signal(SIGINT, set_leave_flag);

  /* client setup */
  int port, c_socket; char ip[100];
  if ((c_socket = client_setup(argc, argv, &port, ip)) < 0) return -1;


  /* get username, color */
  char name[MAX_USERNAME_CHARS + 1] = "\0";
  char color[MAX_COLOR_CHARS + 1] = "\0";


  const int screenWidth = 700;
  const int screenHeight = 700;

  InitWindow(screenWidth, screenHeight, "Eating dots game");

  SetTargetFPS(60);

  int letterCount = 0;

  int font_size = 40;
  int text_box_x_len = font_size * (MAX_CHARS_IN_TEXTBOX - 3.5);
  int text_box_y_len = 50;
  int text_box_x_pos = screenWidth / 2 - text_box_x_len / 2;
  int text_box_y_pos = screenWidth / 2 - text_box_y_len / 2;
  Rectangle textBox = {text_box_x_pos, text_box_y_pos, text_box_x_len, text_box_y_len};

  int framesCounter = 0;

  /* start send thread */
	pthread_t send_thread;
  if(pthread_create(&send_thread, NULL, (void *) send_loop, &c_socket) != 0){
		printf("[ERROR] thread creating err. \n");
    return -1;
	}

  /* start receive thread */
	pthread_t receive_thread;
  if(pthread_create(&receive_thread, NULL, (void *) receive_loop, &c_socket) != 0){
		printf("ERROR: thread creating err. \n");
		return -1;
	}

  /* Main game loop */
  while (!WindowShouldClose() && leave_flag == 0) {
      framesCounter++;

      /* Get pressed key (character) on the queue */
      int key = GetKeyPressed();

      /* Check if more characters have been pressed on the same frame */
      while (key > 0) {
          /* NOTE: Only allow keys in range [32..125] */
          if ((key >= 32) && (key <= 125)) {
              if (letterCount < MAX_USERNAME_CHARS && client_status == 0) {
                  name[letterCount] = (char)key;
                  letterCount++;
              }
              else if (letterCount < MAX_COLOR_CHARS && client_status == 1) {
                  color[letterCount] = (char)key;
                  letterCount++;
              }
          }

          key = GetKeyPressed();  /* Check next character in the queue */
      }

      if (IsKeyPressed(KEY_BACKSPACE)) {
          letterCount--;
          if (letterCount < 0) letterCount = 0;
          if (client_status == 0) name[letterCount] = '\0';
          else if (client_status == 1) color[letterCount] = '\0';
      }

      if (IsKeyPressed(KEY_ENTER)) {
          if (client_status == 0) {
              client_status = 1;
              letterCount = 0;
          } else if (client_status == 1 && letterCount == MAX_COLOR_CHARS) { /* only if color = 6 chars */
              strcpy(client->username, name);
              strcpy(client->color, color);
              client_status = 2;
              letterCount = 0;
              /* sending 0th packet (in send loop), it sends because c_status = 2 */
          }
      }

      int showFromPosition = (letterCount - MAX_CHARS_IN_TEXTBOX > 0) ? letterCount - MAX_CHARS_IN_TEXTBOX : 0;

      /* draw */
      BeginDrawing();

          ClearBackground(RAYWHITE);

          /* draw according to client_status */
          if (client_status == 0) {
              DrawRectangleRec(textBox, LIGHTGRAY);
              DrawText(TextFormat("Please Enter Your Username"), text_box_x_pos, text_box_y_pos - font_size, font_size/2, DARKGRAY);
              DrawText(TextFormat("Input chars: %i/%i", letterCount, MAX_USERNAME_CHARS), text_box_x_pos, text_box_y_pos + 70, font_size/4, DARKGRAY);
              DrawText(&name[showFromPosition], textBox.x + 5, textBox.y + 8, font_size, MAROON);
          }
          else if (client_status == 1) {
              DrawRectangleRec(textBox, LIGHTGRAY);
              DrawText(TextFormat("Please Enter Your Color"), text_box_x_pos, text_box_y_pos - font_size, font_size/2, DARKGRAY);
              DrawText(TextFormat("Input chars: %i/%i", letterCount, MAX_COLOR_CHARS), text_box_x_pos, text_box_y_pos + 70, font_size/4, DARKGRAY);
              DrawText(TextFormat("#%s",&color[showFromPosition]), textBox.x + 5, textBox.y + 8, font_size, MAROON);
          }
          else if (client_status == 2) {
              DrawText(TextFormat("Welcome %s!\nYour color is #%s", name, color), text_box_x_pos, text_box_y_pos, font_size, DARKGRAY);
              DrawText(TextFormat("Sending 0th packet..."), text_box_x_pos, text_box_y_pos + 200, font_size/2, DARKGRAY);
          }
          else if (client_status == 3) {
              DrawText(TextFormat("Welcome %s!\nYour color is #%s", name, color), text_box_x_pos, text_box_y_pos, font_size, DARKGRAY);
              DrawText(TextFormat("Waiting for server accept..."), text_box_x_pos, text_box_y_pos + 200, font_size/2, DARKGRAY);
          }
          else if (client_status > 3) {
              DrawText(TextFormat("Welcome %s!\nYour color is #%s", name, color), text_box_x_pos, text_box_y_pos, font_size, DARKGRAY);
              DrawText(TextFormat("Server accepted you to the game!", name, color), text_box_x_pos, text_box_y_pos + 200, font_size/2, DARKGRAY);
          }

          drawUnderscoreDelMessage(letterCount, client_status, textBox, name, color, font_size, showFromPosition, framesCounter);


      EndDrawing();
  }

  CloseWindow();

  /* close conn */
	close(c_socket);

	return 0;
}
