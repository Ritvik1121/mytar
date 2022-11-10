#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <utime.h>
#include <limits.h>
#include <errno.h>
#define BLOCK 512
#define BACKTWOBLOCK -1024
#define BACKONEBLOCK -512
#define BASE 8
#define USTARNONULL 5
#define STRICT 3
#define VERBOSE 4

#define NAME_OFFSET 0
#define MODE_OFFSET 100
#define UID_OFFSET 108
#define GID_OFFSET 116
#define SIZE_OFFSET 124
#define MTIME_OFFSET 136
#define CHKSUM_OFFSET 148
#define TYPEFLAG_OFFSET 156
#define LINKNAME_OFFSET 157
#define MAGIC_OFFSET 257
#define VERSION_OFFSET 263
#define UNAME_OFFSET 265
#define GNAME_OFFSET 297
#define DEVMAJOR_OFFSET 329
#define DEVMINOR_OFFSET 337
#define PREFIX_OFFSET 345

#define NAME_SIZE 100
#define MODE_SIZE 8
#define UID_SIZE 8
#define GID_SIZE 8
#define SIZE_SIZE 12
#define MTIME_SIZE 12
#define CHKSUM_SIZE 8
#define TYPEFLAG_SIZE 1
#define LINKNAME_SIZE 100
#define MAGIC_SIZE 6
#define VERSION_SIZE 2
#define UNAME_SIZE 32
#define GNAME_SIZE 32
#define DEVMAJOR_SIZE 8
#define DEVMINOR_SIZE 8
#define PREFIX_SIZE 155
#define PATH_SIZE 255

#define RW_PERMS  (S_IRWXU|S_IRWXG|S_IRWXO)
#define X_PERMS (S_IXUSR | S_IXGRP | S_IXOTH)


int make_path (char *path) {
  struct stat sb;
  int len, i, paths = 0;
  char temp[PATH_SIZE] = {0};

  len = strlen(path);

  for (i = 0; i < len; i++) {
    if (path[i] == '/')
      paths++;
  }

  if (paths == 0)
    return 1;

  while (paths != 0) {
    for (i = 0; i < len; i++) {
      temp[i] = path[i]; 
      if (path[i] == '/') {
        paths--;
        if (lstat(temp, &sb) == 0 && S_ISDIR(sb.st_mode))
          continue;
        else {
          if (mkdir(temp, RW_PERMS) == -1) {
            perror("mkdir");
            exit(EXIT_FAILURE);
          }
        }
      }
    }
  }
  return 0;
}

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

void extractTar(int filename, int argc, char* argv[], int flags[]) {
  char name[NAME_SIZE] = {0};
  char mode[MODE_SIZE] = {0};
  char size[SIZE_SIZE] = {0};
  char mtime[MTIME_SIZE] = {0};
  char chksum[CHKSUM_SIZE] = {0};
  char typeflag;
  char linkname[LINKNAME_SIZE] = {0};
  char magic[MAGIC_SIZE] = {0};
  char version[VERSION_SIZE] = {0};
  char prefix[PREFIX_SIZE] = {0};
  char path[PATH_SIZE] = {0};

  struct stat sb;

  int out;

  unsigned long file_mode, filesize, filetime, filechksum;

  unsigned int ensure_cksum = 0;

  int inserted, next, check, i;

  struct utimbuf filetimes;

  char *ptr;
  char *contents;
  char buf[BLOCK] = {0};

  mode_t new_mode;

  char headbuf[BLOCK] = {0};

  while (!(check_end(filename))) {
    check = 0;
    if (read(filename, headbuf, BLOCK) == -1) {
      perror("read");
      exit(EXIT_FAILURE);
    }

    for (i = 0; i < NAME_SIZE; i++) {
      name[i] = headbuf[i];
    }

    for (i = 0; i < MODE_SIZE; i++) {
      mode[i] = headbuf[MODE_OFFSET+i];
    }
    file_mode = strtoul(mode, &ptr, BASE);

    if (file_mode == ULONG_MAX && errno == ERANGE)  {
      fprintf(stderr, "strtoul overflow\n");
      exit(EXIT_FAILURE);
    }

    if ( file_mode & X_PERMS )
      new_mode = RW_PERMS;      
    else
      new_mode = RW_PERMS & ~X_PERMS; 

    for (i = 0; i < SIZE_SIZE; i++) {
      size[i] = headbuf[SIZE_OFFSET+i];
    }
    filesize = strtoul(size, &ptr, BASE);

    if (filesize == ULONG_MAX && errno == ERANGE) {
      fprintf(stderr, "strtoul overflow\n");  
      exit(EXIT_FAILURE);
    }

    for (i = 0; i < MTIME_SIZE; i++) {
      mtime[i] = headbuf[MTIME_OFFSET+i];
    }
    filetime = strtoul(mtime, &ptr, BASE);

    if (filetime == ULONG_MAX && errno == ERANGE) {
      fprintf(stderr, "strtoul overflow\n");
      exit(EXIT_FAILURE);
    }

    for (i = 0; i < CHKSUM_SIZE; i++) {
      chksum[i] = headbuf[CHKSUM_OFFSET+i];
    }
    filechksum = strtoul(chksum, &ptr, BASE);

    if (filechksum == ULONG_MAX && errno == ERANGE) {
      fprintf(stderr, "strtoul overflow\n");
      exit(EXIT_FAILURE);
    }

    typeflag = headbuf[TYPEFLAG_OFFSET];

    if(typeflag == '2') {
      for (i = 0; i < LINKNAME_SIZE; i++) {
        linkname[i] = headbuf[LINKNAME_OFFSET+i]; 
    }
    }

    for (i = 0; i < MAGIC_SIZE; i++) {
        magic[i] = headbuf[MAGIC_OFFSET+i];
    }

    for (i = 0; i < VERSION_SIZE; i++) {
        version[i] = headbuf[VERSION_OFFSET+i];
    }

    for (i = 0; i < PREFIX_SIZE; i++) {
      prefix[i] = headbuf[PREFIX_OFFSET+i];
    }

    if (prefix[0] != '\0') {
      strcpy(path, prefix);
      strcat(path, "/");
      strcat(path, name);
    }
    else
      strcpy(path, name);
  
    if (argc > 3) {
      for (i = 3; i < argc; i++) {
      if (strstr(path, argv[i]) != NULL) {
        check = 1;
      }
    }
    }
    else {
      check = 1;
    }
    
    if (check == 1) {
    if (strncmp("ustar", magic, USTARNONULL) != 0) {
      fprintf(stderr, "bad magic number\n");
      exit(EXIT_FAILURE);
    }

    if (flags[STRICT] == 1) {
      if (magic[strlen(magic)] != '\0') {
        fprintf(stderr, "bad magic number\n");
        exit(EXIT_FAILURE);
        }
      if (strcmp(version, "00") != 0) {
        fprintf(stderr, "bad verison\n");
        exit(EXIT_FAILURE);
        }
    }

    if (lseek(filename, BACKONEBLOCK, SEEK_CUR) == -1) {
      perror("lseek");
      exit(EXIT_FAILURE);
    }

    if (read(filename, buf, BLOCK) == -1) {
      perror("read");
      exit(EXIT_FAILURE);
    }

    ensure_cksum = checksum(buf, BLOCK);

    if (filechksum != ensure_cksum) {
      fprintf(stderr, "bad checksum\n");
      exit(EXIT_FAILURE);
    }

    if (typeflag == '5') {
      make_path(path);
      if (mkdir(path, RW_PERMS) == -1) {
        perror("mkdir");
        exit(EXIT_FAILURE);
      }
    }

    else if (typeflag == '2') {
      make_path(path);
      symlink(linkname, path);
    }
    else {
      make_path(path);
      if ((out = open(path, O_RDWR | O_CREAT | O_TRUNC, new_mode)) == -1) {
        perror("open");
        exit(EXIT_FAILURE);
      }
    }

    if (flags[VERBOSE] == 1) {
      printf("%s\n", path);
    }

    contents = malloc(filesize);
    if (contents == NULL) {
      perror("malloc");
      exit(EXIT_FAILURE);
    }

    if (read(filename, contents, filesize) == -1) {
      perror("read");
      exit(EXIT_FAILURE);
    }

    if (write(out, contents, filesize) == -1) {
      perror("write");
      exit(EXIT_FAILURE);
    }

    free(contents);

    if (lstat(path, &sb) == -1) {
      perror("lstat");
      exit(EXIT_FAILURE);
    }

    filetimes.actime = sb.st_atime;
    filetimes.modtime = filetime;

    if (utime(path, &filetimes) == -1) {
      perror("utime");
      exit(EXIT_FAILURE);
    }

    inserted = filesize % BLOCK;
    if (inserted != 0){
        next = BLOCK - inserted;
        if (lseek(filename, next, SEEK_CUR) == 1) {
          perror("lseek");
          exit(EXIT_FAILURE);
        }
    }
    if (close(out) == -1) {
      perror("close");
      exit(EXIT_FAILURE);      
    }
    }
  }
}
