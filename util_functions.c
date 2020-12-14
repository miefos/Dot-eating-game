#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h> /* toupper, isprint */
#include <inttypes.h>
#include <unistd.h>
#include <time.h>
#include "util_functions.h"
#include "setup.h"

int nsleep(long miliseconds) {
    struct timespec req, rem;

    if(miliseconds > 999)
    {
        req.tv_sec = (int)(miliseconds / 1000);                            /* Must be Non-Negative */
        req.tv_nsec = (miliseconds - ((long)req.tv_sec * 1000)) * 1000000; /* Must be in range of 0 to 999999999 */
    }
    else
    {
        req.tv_sec = 0;                         /* Must be Non-Negative */
        req.tv_nsec = miliseconds * 1000000;    /* Must be in range of 0 to 999999999 */
    }

    return nanosleep(&req , &rem);
}

unsigned char get_checksum(unsigned char* arr, int size) {
  unsigned char chk = 0;
  int i;
  for (i = 0; i < size; i++) chk ^= arr[i];
  return chk;
}

int is_little_endian_system() {
  volatile uint32_t i = 0x01234567;
  return (*((uint8_t*)(&i))) == 0x67;
  /* 1 = little, 0 = big*/
}

void remove_newline(char *str) {
      if (strlen(str) > 0 && str[strlen(str)-1] == '\n')
      	str[strlen(str)-1] = '\0';
}

int contains_only_hex_digits(char* str) {
  int i;
  for (i = 0; str[i] != '\0'; i++) {
    char c = toupper(str[i]);
    if ((c < 48 || c > 57) && /* decimal digit */
        (c < 65 || c > 70)) { /* 'A' - 'F' */
        return str[i]; /* invalid char */
    } else {
      str[i] = c; /* make uppercase */
    }
  }

  return -1; /* yes, contains only hex digits */
}

int assign_int_to_bytes_lendian_escape(unsigned char* packet_part, int n, int should_escape) {
  int escape_count = 0;
  if (should_escape) {
    unsigned char byte;
    /* byte 1 */
    byte = n & 0xFF;
    escape_count += escape_assign(byte, &packet_part[0]);
    /* byte 2 */
    byte = (n >> 8) & 0xFF;
    escape_count += escape_assign(byte, &packet_part[1 + escape_count]);
    /* byte 3 */
    byte = (n >> 16) & 0xFF;
    escape_count += escape_assign(byte, &packet_part[2 + escape_count]);
    /* byte 4 */
    byte = (n >> 16) & 0xFF;
    escape_count += escape_assign(byte, &packet_part[3 + escape_count]);
  } else {
    packet_part[0] = n & 0xFF;
    packet_part[1] = (n >> 8) & 0xFF;
    packet_part[2] = (n >> 16) & 0xFF;
    packet_part[3] = (n >> 24) & 0xFF;
  }

  return escape_count;
}

int get_int_from_4bytes_lendian(unsigned char* the4bytes) {
  /* It works only for little endian... */
  return *((int *) the4bytes);
}

short int get_sh_int_2bytes_lendian(unsigned char* the2bytes) {
  /* It works only for little endian... */
  return *((short int *) the2bytes);
}


void print_one_byte (unsigned char byte) {
  printf("Byte %c (%d)\n", printable_char(byte), byte);
}


int escape_assign(unsigned char num, unsigned char* packet) {
  if (num == 0) {
    packet[0] = 1;
    packet[1] = 2;
    return 1;

  } else if (num == 1) {
    packet[0] = 1;
    packet[1] = 3;
    return 1;

  } else {
    packet[0] = num & 0xFF; /* cuts to one byte */
  }

  return 0;
}

char printable_char(char c) {
  if (isprint(c)) return c;
  return ' ';
}


void print_bytes(void* packet, int count) {
  int i = 0;
  unsigned char *p = (unsigned char *) packet;

  printf("Printing %d bytes...\n", count);
  printf("[NPK] [C] [HEX] [DEC] [ BINARY ]    [ADDRESS]\n");
  printf("===============================================\n");
  for (i = 0; i < count; i++) {
    printf(" %3d | %c | %02x | %3d | %c%c%c%c%c%c%c%c | %p\n", i, printable_char(p[i]), p[i], p[i],
      p[i] & 0x80 ? '1' : '0',
      p[i] & 0x40 ? '1' : '0',
      p[i] & 0x20 ? '1' : '0',
      p[i] & 0x10 ? '1' : '0',
      p[i] & 0x08 ? '1' : '0',
      p[i] & 0x04 ? '1' : '0',
      p[i] & 0x02 ? '1' : '0',
      p[i] & 0x01 ? '1' : '0',
      &p[i]
    );
  }
}

/* bitNumber should be from left side */
/* Ex: (0000 0100 to get 1 we should indicate bitNumber 2) */
char get_bit(unsigned char byte, char bitNumber) {
  return (byte >> bitNumber) & 1; /*0 or 1*/
}
