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
#define BLOCK 512

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
#define PADDING_SIZE 12
#define PATH_SIZE 255

uint32_t extract_special_int (char *where, int len) {
/* For interoperability with GNU tar. GNU seems to
* set the high–order bit of the first byte, then
* treat the rest of the field as a binary integer
* in network byte order.
* I don’t know for sure if it’s a 32 or 64–bit int, but for
* this version, we’ll only support 32. (well, 31)
* returns the integer on success, –1 on failure.
* In spite of the name of htonl(), it converts int32 t
*/
int32_t val= -1;
if ( (len >= sizeof(val)) && (where[0] & 0x80)) {
/* the top bit is set and we have space
* extract the last four bytes */
val = *(int32_t *)(where+len-sizeof(val));
val = ntohl(val); /* convert to host byte order */
}
return val;
}

int make_path (char *path) {
  struct stat sb;
  int len, i, paths = 0;
  char temp[255] = {0};

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
        if (stat(temp, &sb) == 0 && S_ISDIR(sb.st_mode))
          continue;
        else {
          mkdir(temp, S_IRWXU);
        }
      }
    }
  }
  return 0;
}

int check_end(int blocks, char* magic, char* chksum) {
  

}

/* file has execute give everyone rwx else give everyone rw*/

void extractTar(int filename, int argc, char* argv[], int flags[]) {
  char name[NAME_SIZE] = {0};
  char mode[MODE_SIZE] = {0};
  char uid[UID_SIZE] = {0};
  char gid[GID_SIZE] = {0};
  char size[SIZE_SIZE] = {0};
  char mtime[MTIME_SIZE] = {0};
  char chksum[CHKSUM_SIZE] = {0};
  char typeflag;
  char linkname[LINKNAME_SIZE] = {0};
  char magic[MAGIC_SIZE] = {0};
  char version[VERSION_SIZE] = {0};
  char uname[UNAME_SIZE] = {0};
  char gname[GNAME_SIZE] = {0};
  char prefix[PREFIX_SIZE] = {0};
  char path[PATH_SIZE] = {0};

  struct stat sb;

  int out;

  long file_mode, user, group, filesize, filetime, filechksum;

  int inserted, next, done = 0, check, count = 0;

  time_t mod_time;

  char *ptr;
  char *contents;

  u_int32_t spec_uid;

  int i = 3;

  while (done == 0) {
    count++;
    check = 0;
    read(filename, name, NAME_SIZE);
      if (name[0] == '\0') {
        done = 1;
        break;
      }

    read(filename, mode, MODE_SIZE);
    file_mode = strtoul(mode, &ptr, 8);

    read(filename, uid, UID_SIZE);

    spec_uid = extract_special_int(uid, UID_SIZE);
    user = spec_uid;

    read(filename, gid, GID_SIZE);
    group = strtoul(gid, &ptr, 8);

    read(filename, size, SIZE_SIZE);
    filesize = strtoul(size, &ptr, 8);

    read(filename, mtime, MTIME_SIZE);
    filetime = strtol(mtime, &ptr, 8);
    mod_time = filetime;

    read(filename, chksum, CHKSUM_SIZE);
    filechksum = strtol(chksum, &ptr, 8);

    read(filename, &typeflag, TYPEFLAG_SIZE);

    if(typeflag == '2') {
      read(filename, linkname, LINKNAME_SIZE);
    }
    else {
      lseek(filename, LINKNAME_SIZE, SEEK_CUR);
    }

    read(filename, magic, MAGIC_SIZE);
    read(filename, version, VERSION_SIZE);

    if (flags[4] == 1) {
      if (strcmp(magic, "ustar\0") != 0 || strcmp(version, "00") != 0)
        printf("s-invalid");
    }

    read(filename, uname, UNAME_SIZE);
    read(filename, gname, GNAME_SIZE);

    /*
    printf("%s\n", name);
    printf("%ld\n", file_mode);
    printf("%" PRIu32 "\n",spec_uid);
    printf("%ld\n", group);
    printf("%ld\n", filesize);
    printf("%ld\n", filechksum);
    printf("%c\n", typeflag);
    printf("%s\n", magic);
    printf("%c%c\n", version[0],version[1]);
    printf("%s\n", uname);
    printf("%s\n", gname);

    fix if given a file in command line
    */

    lseek(filename, 16, SEEK_CUR);

    read(filename, prefix, PREFIX_SIZE);

    if (prefix[0] != '\0') {
      strcpy(path, prefix);
      strcat(path, "/");
      strcat(path, name);
    }
    else
      strcpy(path, name);
  
    lseek(filename, PADDING_SIZE, SEEK_CUR);

    if (typeflag == '5') {
      make_path(path);
      mkdir(path, file_mode);
    }

    else if (typeflag == '2') {
      make_path(path);
      symlink(linkname, path);
    }

    else {
      make_path(path);
      out = open(path, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    }

    if (flags[3] == 1) {
      printf("%s\n", path);
    }

    contents = malloc(filesize);

    read(filename, contents, filesize);

    write(out, contents, filesize);

    free(contents);

    chmod(path, file_mode);

    chown(path, user, group);

    stat(path, &sb);

    inserted = filesize % 512;
    if (inserted != 0){
        next = BLOCK - inserted;
        lseek(filename, next, SEEK_CUR);
    }
    close(out);
    }
  close(filename);
}

void extractFiles(int filename, int argc, char* argv[], int flags[]) {
  char name[NAME_SIZE] = {0};
  char mode[MODE_SIZE] = {0};
  char uid[UID_SIZE] = {0};
  char gid[GID_SIZE] = {0};
  char size[SIZE_SIZE] = {0};
  char mtime[MTIME_SIZE] = {0};
  char chksum[CHKSUM_SIZE] = {0};
  char typeflag;
  char linkname[LINKNAME_SIZE] = {0};
  char magic[MAGIC_SIZE] = {0};
  char version[VERSION_SIZE] = {0};
  char uname[UNAME_SIZE] = {0};
  char gname[GNAME_SIZE] = {0};
  char prefix[PREFIX_SIZE] = {0};
  char path[PATH_SIZE] = {0};

  struct stat sb;

  int out;

  long file_mode, user, group, filesize, filetime, filechksum;

  int inserted, next, done = 0, check;

  time_t mod_time;

  char *ptr;
  char *contents;

  u_int32_t spec_uid;

  int i = 3;

  while (done == 0) {
    check = 0;
    read(filename, name, NAME_SIZE);
      if (name[0] == '\0') {
        done = 1;
        break;
      }

    read(filename, mode, MODE_SIZE);
    file_mode = strtoul(mode, &ptr, 8);

    read(filename, uid, UID_SIZE);

    spec_uid = extract_special_int(uid, UID_SIZE);
    user = spec_uid;

    read(filename, gid, GID_SIZE);
    group = strtoul(gid, &ptr, 8);

    read(filename, size, SIZE_SIZE);
    filesize = strtoul(size, &ptr, 8);

    read(filename, mtime, MTIME_SIZE);
    filetime = strtol(mtime, &ptr, 8);
    mod_time = filetime;

    read(filename, chksum, CHKSUM_SIZE);
    filechksum = strtol(chksum, &ptr, 8);

    read(filename, &typeflag, TYPEFLAG_SIZE);

    if(typeflag == '2') {
      read(filename, linkname, LINKNAME_SIZE);
    }
    else {
      lseek(filename, LINKNAME_SIZE, SEEK_CUR);
    }

    read(filename, magic, MAGIC_SIZE);
    read(filename, version, VERSION_SIZE);

    if (flags[4] == 1) {
      if (strcmp(magic, "ustar\0") != 0 || strcmp(version, "00") != 0)
        printf("s-invalid");
    }

    read(filename, uname, UNAME_SIZE);
    read(filename, gname, GNAME_SIZE);

    lseek(filename, 16, SEEK_CUR);

    read(filename, prefix, PREFIX_SIZE);

    if (prefix[0] != '\0') {
      strcpy(path, prefix);
      strcat(path, "/");
      strcat(path, name);
    }
    else
      strcpy(path, name);

    lseek(filename, PADDING_SIZE, SEEK_CUR);


    for (i = 3; i < argc; i++) {
      if (strstr(path, argv[i]) != NULL) {
        check = 1;
      }
    }

    if (check == 1) {
    if (typeflag == '5') {
      make_path(path);
      mkdir(path, file_mode);
    }

    else if (typeflag == '2') {
      make_path(path);
      symlink(linkname, path);
    }

    else if (typeflag == '0') {
      make_path(path);
      out = open(path, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    }

    if (flags[3] == 1) {
      printf("%s\n", path);
    }

    contents = malloc(filesize);

    read(filename, contents, filesize);

    write(out, contents, filesize);

    free(contents);

    chmod(path, file_mode);

    chown(path, user, group);

    stat(path, &sb);

    inserted = filesize % 512;
    if (inserted != 0){
        next = BLOCK - inserted;
        lseek(filename, next, SEEK_CUR);
    }
    close(out);
    }
    }
  close(filename);
}