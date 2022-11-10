#include "headers.h"

int check_end(int filename) {
  char block1[BLOCK] = {0};
  char block2[BLOCK] = {0};
  int i = 0, done = 1;

  if (read(filename, block1, BLOCK) == -1) {
      perror("read");
      exit(EXIT_FAILURE);
  }

  if (read(filename, block2, BLOCK) == -1) {
      perror("read");
      exit(EXIT_FAILURE);
  }

  for (i=0; i < BLOCK; i++) {
      if ( block1[i] != '\0' ) {
        done = 0;
        break;
      }
    }
  for (i=0; i < BLOCK; i++) {
      if ( block2[i] != '\0' ) {
        done = 0;
        break;
      }
    }

  if (!done)
    if (lseek(filename, BACKTWOBLOCK, SEEK_CUR) == 1) {
      perror("lseek");
      exit(EXIT_FAILURE);
    }

  return done;
}

unsigned int checksum(char *buffer, int length) {
  unsigned int sum = 0;
  int i;
  for (i=0; i < length; i++ )
    sum += (unsigned char) buffer[i];

  for (i=0; i < CHKSUM_SIZE; i++) {
    sum -= buffer[CHKSUM_OFFSET + i];
    sum += ' ';
  }

  return sum;
}
