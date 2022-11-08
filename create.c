/* Ritvik Durgempudi, Jinrong Pettit, Wesley Tam
 * November 9, 2022
 * CSC-357 Nico */

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
#include <dirent.h>
#include <errno.h>
#include <math.h>

#define MAGIC "ustar\0"
#define VERSION "00"
#define BLKSIZE 512

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
#define POST_HEADER_SIZE 12

#define CHKSUM_OFFSET 148

void sys_error(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

void safe_write(int fd, const void *buf, size_t count) {
    if (write(fd, buf, count) == -1)
        sys_error("write");
}

int insert_special_int(char *where, size_t size, int32_t val) {
    /* For interoperability with GNU tar. GNU seems to
     * set the high–order bit of the first byte, then
     * treat the rest of the field as a binary integer
     * in network byte order.
     * Insert the given integer into the given field
     * using this technique. Returns 0 on success, nonzero
     * otherwise
     * */
    int err = 0;
    if (val < 0 || (size < sizeof(val))) {
        /* if it’s negative, bit 31 is set and we can’t use the flag
         * if len is too small, we can’t write it. Either way, we’re
         * done.
         * */
        err++;
    } else {
        /* game on... */
        memset(where, 0, size); /* Clear out buffer */
        *(int32_t *)(where+size-sizeof(val)) = htonl(val);  /* place the int */
        *where |= 0x80; /* set the high-order bit */
    }
    return err;
}

int decToOctal(int num, char *buffer, int length, int flags[6]) {
    /* Converts decimal num into base octal, stores it as string into buffer 
     * If num is too large to represent, uses insert_special_int() unless
     * Strict flag is on, in which NULL is returned */
    unsigned long i, max = 0;

    for (i = 0; i < (length - 1); i++) {
        /* Calculate max decimal number [length] octal digits can represent */
        max += (7 * pow(8, i));
    }
    if (num > max) {
        if (flags[4] == 1) {
            printf("octal value too long. (0%o)\n", num);
            return -1;
        }
        if (insert_special_int(buffer, length, num) != 0)
            sys_error("number too large");    /* Find better error handling */
        return 1;
    }
    snprintf(buffer, length, "%0*o", length - 1, num);  /* length-1 for null */
    return 0;
}

unsigned int checksum(char *buffer, int length) {
    /* Calculates checksum from given buffer */
    unsigned int sum = 0;
    int i;

    for (i = 0; i < length; i++ )
        sum += (unsigned char) buffer[i];
    for (i = 0; i < CHKSUM_SIZE; i++) {
        sum -= buffer[CHKSUM_OFFSET + i];
        sum += ' ';
    }
    return sum;
}

int write_header(struct stat st, int tarfile, char *path, int flags[6]) {
    /* Takes in a struct stat and path string, writes the header block 
     * Return value of 0: No error
     * Return value of 1: path is too long to fit in name and prefix fields
     * Return value of 2: number is too long to be represented in octal */
    unsigned int j, namelen, prefixlen, fits, chksum = 0;
    struct passwd *pwd;
    struct group *grp;
    char spaces[CHKSUM_SIZE] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    int zeroes[DEVMAJOR_SIZE + DEVMINOR_SIZE] = {0};
    char name[NAME_SIZE] = {'\0'};
    char prefix[PREFIX_SIZE] = {'\0'};
    char linkname[LINKNAME_SIZE] = {'\0'};
    char uname[UNAME_SIZE] = {'\0'}, gname[GNAME_SIZE] = {'\0'};
    char *buf;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    off_t size;
    time_t mtime;
    char typeflag;

    namelen = prefixlen = 0;
    if (strlen(path) <= NAME_SIZE) {
        for (namelen = 0; namelen < strlen(path); namelen++)
            name[namelen] = path[namelen];
        if (namelen<NAME_SIZE && S_ISDIR(st.st_mode) && name[namelen-1] != '/')
            name[namelen++] = '/';
    } else {
        fits = 0;
        while (fits == 0) {
            while (path[prefixlen] != '/' && prefixlen < PREFIX_SIZE) {
                /* Copy path into prefix until path can fit into name */
                prefix[prefixlen] = path[prefixlen];
                prefixlen++;
            }
            if (prefixlen >= PREFIX_SIZE) {
                printf("%s: unable to construct header.  ", path);
                printf("(Name too long?) Skipping.\n");
                return 1;
            } else if ((strlen(path) - prefixlen) < NAME_SIZE) {
                fits = 1;
                break;
            }
            if (prefixlen > 0 && prefix[prefixlen - 1] != '/')
                prefix[prefixlen++] = '/';
        }
        memcpy(name, path + prefixlen + 1, strlen(path) - prefixlen);
        /* (prefixlen + 1) to skip the slash in between prefix and name */
        namelen = strlen(path) - (prefixlen + 1);
        /* (prefixlen + 1) to include the '/' skipped over */
        if (namelen > 0 && namelen < NAME_SIZE && S_ISDIR(st.st_mode)) {
            if (name[namelen - 1] != '/')
                name[namelen] = '/';
        }
    }

    mode = st.st_mode;
    uid = st.st_uid;
    gid = st.st_gid;
    size = st.st_size;
    mtime = st.st_mtime;
    if ((pwd = getpwuid(uid)) == NULL) {
        sys_error("getpwuid");
    }
    for (j = 0; j < strlen(pwd->pw_name); j++) {
        uname[j] = pwd->pw_name[j]; /* May not be null-terminated */
    }
    if ((grp = getgrgid(gid)) == NULL) {
        sys_error("getgrgid");
    }
    for (j = 0; j < strlen(grp->gr_name); j++) {
        gname[j] = grp->gr_name[j]; /* May not be null-terminated */
    }
    if (S_ISLNK(mode)) {
        typeflag = '2';
        if (readlink(path, linkname, sizeof(linkname)) == -1)
            sys_error("readlink");
        /* If name doesn't fit, readlink puts first n bytes */
    } else if (S_ISREG(mode)) {
        typeflag = '0';
    } else if (S_ISDIR(mode)) {
        typeflag = '5';
    } else {
        /* Regular file (alternate)? */
        typeflag = '\0';
    }

    safe_write(tarfile, name, NAME_SIZE);

    if ((buf = (char *)malloc(sizeof(char) * MODE_SIZE)) == NULL)
        sys_error("malloc");
    mode &= 0xFFF;  /* Only care about last 12 bits for permission */
    if (decToOctal(mode, buf, MODE_SIZE, flags) == -1) {
        printf("%s: Unable to create conforming header.  Skipping.\n", path);
        return 2;
    }
    safe_write(tarfile, buf, MODE_SIZE);
    free(buf);

    if ((buf = (char *)malloc(sizeof(char) * UID_SIZE)) == NULL)
        sys_error("malloc");
    if (decToOctal(uid, buf, UID_SIZE, flags) == -1) {
        printf("%s: Unable to create conforming header.  Skipping.\n", path);
        return 2;
    }
    safe_write(tarfile, buf, UID_SIZE);
    free(buf);

    if ((buf = (char *)malloc(sizeof(char) * GID_SIZE)) == NULL)
        sys_error("malloc");
    if (decToOctal(gid, buf, GID_SIZE, flags) == -1) {
        printf("%s: Unable to create conforming header.  Skipping.\n", path);
        return 2;
    }
    safe_write(tarfile, buf, GID_SIZE);
    free(buf);

    if ((buf = (char *)malloc(sizeof(char) * SIZE_SIZE)) == NULL)
        sys_error("malloc");
    if (S_ISDIR(st.st_mode) || S_ISLNK(st.st_mode))
        size = 0;
    if (decToOctal(size, buf, SIZE_SIZE, flags) == -1) {
        printf("%s: Unable to create conforming header.  Skipping.\n", path);
        return 2;
    }
    safe_write(tarfile, buf, SIZE_SIZE);
    free(buf);

    if ((buf = (char *)malloc(sizeof(char) * MTIME_SIZE)) == NULL)
        sys_error("malloc");
    if ((decToOctal(mtime, buf, MTIME_SIZE, flags)) == -1) {
        printf("%s: Unable to create conforming header.  Skipping.\n", path);
        return 2;
    }
    safe_write(tarfile, buf, MTIME_SIZE);
    free(buf);

    safe_write(tarfile, spaces, CHKSUM_SIZE);    /* temporary checksum */

    safe_write(tarfile, &typeflag, TYPEFLAG_SIZE);

    safe_write(tarfile, &linkname, LINKNAME_SIZE);

    safe_write(tarfile, MAGIC, MAGIC_SIZE);

    safe_write(tarfile, VERSION, VERSION_SIZE);

    safe_write(tarfile, uname, UNAME_SIZE);

    safe_write(tarfile, gname, GNAME_SIZE);

    safe_write(tarfile, zeroes, DEVMAJOR_SIZE + DEVMINOR_SIZE);

    safe_write(tarfile, prefix, PREFIX_SIZE);

    safe_write(tarfile, zeroes, POST_HEADER_SIZE);

    if (lseek(tarfile, -BLKSIZE, SEEK_CUR) == -1) /* Beginning of header */
        sys_error("lseek");
    if ((buf = (char *)malloc(sizeof(char) * BLKSIZE)) == NULL)
        sys_error("malloc");
    if (read(tarfile, buf, BLKSIZE) == -1)
        sys_error("read");
    chksum = checksum(buf, BLKSIZE);
    free(buf);
    if ((buf = (char *)malloc(sizeof(char) * CHKSUM_SIZE)) == NULL)
        sys_error("malloc");
    if (decToOctal(chksum, buf, CHKSUM_SIZE, flags) == -1) {
        printf("%s: Unable to create conforming header.  Skipping.\n", path);
        return 2;
    }
    if (lseek(tarfile, -BLKSIZE + CHKSUM_OFFSET, SEEK_CUR) == -1)
        /* lseek to checksum offset */
        sys_error("lseek");
    safe_write(tarfile, buf, CHKSUM_SIZE);
    free(buf);
    return 0;
}

void write_data(int tarfile, char *filename) {
    /* Write file data into tar file */
    int i, file, num_bytes;
    char *buf;

    if ((file = open(filename, O_RDONLY)) == -1)
        sys_error("open");
    if ((buf = (char *)malloc(BLKSIZE * sizeof(char))) == NULL)
        sys_error("malloc");
    while ((num_bytes = read(file, buf, BLKSIZE)) > 0) {
        if (num_bytes < BLKSIZE) {
            for (i = num_bytes; i < BLKSIZE; i++) {
                buf[i] = '\0';
            }
        }
        safe_write(tarfile, buf, BLKSIZE);
    }
    if (num_bytes == -1)
        sys_error("write");
    free(buf);
    close(file);
}

int _create_helper(int tarfile, char *filename, int flags[6]) {
    struct stat st;
    char *dir_name, *new_path;
    DIR *dir_ptr;
    struct dirent *dir;

    if (lstat(filename, &st) == -1) {
        /* Cannot stat file, continue to next one */
        fprintf(stderr, "%s: No such file or directory\n", filename);
        return 1;
    }

    if (write_header(st, tarfile, filename, flags) == 0) {
        if (flags[3] == 1) {
            /* Verbose flag is on */
            printf("%s", filename);
            if (S_ISDIR(st.st_mode) && filename[strlen(filename) - 1] != '/')
                printf("/");
            printf("\n");
        }
    }

    if (lseek(tarfile, 0, SEEK_END) == -1)
        sys_error("lseek");
    if (S_ISREG(st.st_mode)) {
        /* Write data in file */
        write_data(tarfile, filename);
    } else if (S_ISDIR(st.st_mode)) {
        if ((dir_ptr = opendir(filename)) == NULL)
            sys_error("opendir");
        errno = 0;
        while ((dir = readdir(dir_ptr)) != NULL) {
            if (strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")) {
                if (!(dir_name = strndup(dir->d_name, strlen(dir->d_name))))
                    sys_error("strndup");
                if ((new_path = (char *)calloc(sizeof(char), 
                        (strlen(filename) + strlen(dir->d_name) + 2))) == NULL)
                    sys_error("calloc");
                /* +2 To fit '/' and '\0' */
                memcpy(new_path, filename, strlen(filename));
                if (strlen(filename) > 0 && 
                        new_path[strlen(filename) - 1] != '/') {
                    new_path[strlen(filename)] = '/';
                    memcpy(new_path + strlen(filename) + 1, 
                            dir_name, strlen(dir_name));
                    /* +1 To account for '/' */
                } else {
                    memcpy(new_path + strlen(filename), 
                            dir_name, strlen(dir_name));
                }
                _create_helper(tarfile, new_path, flags);
                free(new_path);
                free(dir_name);
            }
        }
        if (errno != 0)
            sys_error("readdir");
        if (closedir(dir_ptr) == -1)
            sys_error("closedir");
    }
    return 0;
}

void createTar(int argc, char **argv, int flags[6]) {
    /* Create file for tar */
    int i, tarfile;
    char *buf;

    if ((tarfile = open(argv[2], O_CREAT | O_TRUNC | O_RDWR, S_IRWXU)) == -1)
        sys_error("open");
    for (i = 3; i < argc; i++) {
        /* Loop through path(s) */
        _create_helper(tarfile, argv[i], flags);
    }
    if ((buf = (char *)calloc(BLKSIZE, sizeof(char))) == NULL)
        sys_error("calloc");
    for (i = 0; i < 2; i++) {
        /* 2 stop blocks */
        safe_write(tarfile, buf, BLKSIZE);
    }
    free(buf);
    close(tarfile);
}