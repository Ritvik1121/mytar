#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <dirent.h>
#include <errno.h>
#include <utime.h>
#include <limits.h>
#define BLOCK 512
#define BACKTWOBLOCK -1024

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

#define POST_HEADER_SIZE 12
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
#define MAGIC "ustar\0"
#define VERSION "00"

#define DEC 8
#define OUTTIME_SIZE 20

#define RW_PERMS  (S_IRWXU|S_IRWXG|S_IRWXO)
#define X_PERMS (S_IXUSR | S_IXGRP | S_IXOTH)

char *find_permissions(long permissions, char *type);
int table_mode(int file, int *flags, int arg, char **args);
int check_end(int filename);
unsigned int checksum(char *buffer, int length);
void extractTar(int filename, int argc, char* argv[], int flags[]);
void createTar(int argc, char **argv, int flags[6]);
