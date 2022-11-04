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

#define MAGIC "ustar\0"
#define VERSION "00"
#define BLKSIZE 512

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

char *decToOctal(int num, char *buffer, int length) {
    int i;

    for (i = length - 1; num != 0; i--) {
        buffer[i] = (num % 8) + '0';
        num = num / 8;
    }
    while (i >= 0)
        buffer[i--] = '0';
    return buffer;
}

void parse_cmd(int argc, char **argv, int flags[6]) {
    /* Parses the command line arguments
        * Checks for user usage error, sets flags for program */
    int i;

    if (argc < 3) {
        fprintf(stderr, "usage: %s [ctxvS]f tarfile [ path [ ... ] ]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < strlen(argv[1]); i++) {
        if (argv[1][i] == 'c' && flags[0] != 1)
            flags[0] = 1;
        else if (argv[1][i] == 't' && flags[1] != 1)
            flags[1] = 1;
        else if (argv[1][i] == 'x' && flags[2] != 1)
            flags[2] = 1;
        else if (argv[1][i] == 'v' && flags[3] != 1)
            flags[3] = 1;
        else if (argv[1][i] == 'S' && flags[4] != 1)
            flags[4] = 1;
        else if (argv[1][i] == 'f' && flags[5] != 1)
            flags[5] = 1;
        else {
            fprintf(stderr, "usage: %s [ctxvS]f tarfile [ path [ ... ] ]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    if (flags[0] != 1 && flags[1] != 1 && flags[2] != 1 && flags[5] != 1) {
        /* Flag c, t, x, or f are necessary */
        fprintf(stderr, "%s: you must specify at least one of the 'ctx' options.", argv[0]);
        fprintf(stderr, "usage: %s [ctxvS]f tarfile [ path [ ... ] ]\n", argv[0]);
        exit(EXIT_FAILURE);
    } else if ((flags[0] == 1 && (flags[1] == 1 || flags[2] == 1)) ||
                (flags[1] == 1 && (flags[0] == 1 || flags[2] == 1)) ||
                (flags[2] == 1 && (flags[0] == 1 || flags[1] == 1))) {
        /* Flags c, t, and x cannot be used together */
        fprintf(stderr, "%s: you must choose one of the 'ctx' options.", argv[0]);
        fprintf(stderr, "usage: %s [ctxvS]f tarfile [ path [ ... ] ]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
}

void tar_create(int argc, char **argv, int flags[6]) {
    /* Create file for tar */
    unsigned int i, j, n, namelen, tarfile, file, chksum = 0;
    struct stat st;
    struct passwd *pwd;
    struct group *grp;
    char spaces[8] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    int zeroes[16] = {0};
    char name[100] = {'\0'}, prefix[155] = {'\0'}, linkname[100] = {'\0'};
    char uname[32] = {'\0'}, gname[32] = {'\0'};        /* NULL-terminated */
    char *buf;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    off_t size;
    time_t mtime;
    char typeflag, nul = '\0';
    uint8_t *chksum_ptr;

    tarfile = open(argv[2], O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
    for (i = 3; i < argc; i++) {
        /* Loop through path(s) */
        if (lstat(argv[i], &st) == -1)
            sys_error("lstat");

        for (namelen = 0; namelen < strlen(argv[i]) && namelen < 100; namelen++) {
            name[namelen] = argv[i][namelen];
        }
        if (namelen == 100) {
            for (; namelen < strlen(argv[i]) || namelen < 255; namelen++) {
                prefix[namelen - 155] = argv[i][namelen];
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
            uname[j] = pwd->pw_name[j]; // May not be null-terminated
        }
        if ((grp = getgrgid(gid)) == NULL) {
            sys_error("getgrgid");
        }
        for (j = 0; j < strlen(grp->gr_name); j++) {
            gname[j] = grp->gr_name[j]; // May not be null-terminated
        }
        if (S_ISLNK(mode)) {
            typeflag = '2';
            if (readlink(argv[i], linkname, sizeof(linkname)) == -1)
                sys_error("readlink");
                // If name doesn't fit, readlink puts first n bytes in buffer
        } else if (S_ISREG(mode)) {
            typeflag = '0';
        } else if (S_ISDIR(mode)) {
            typeflag = '5';
        } else {
            // Regular file (alternate)?
            typeflag = '\0';
        }

        safe_write(tarfile, name, 100);

        buf = (char *)malloc(sizeof(char) * 7);
        mode &= 0xFFF;  // Only care about last 12 bits for permission
        decToOctal(mode, buf, 7) != 0;
        /* If number too large, use insert_special_int instead */
        safe_write(tarfile, buf, 7);
        safe_write(tarfile, &nul, 1);
        free(buf);

        buf = (char *)malloc(sizeof(char) * 8);
        if (insert_special_int(buf, 8, uid) != 0)
            sys_error("number too large");
        safe_write(tarfile, buf, 8);
        free(buf);

        buf = (char *)malloc(sizeof(char) * 7);
        buf = decToOctal(gid, buf, 7);
        safe_write(tarfile, buf, 7);
        safe_write(tarfile, &nul, 1);
        free(buf);

        buf = (char *)malloc(sizeof(char) * 11);
        buf = decToOctal(size, buf, 11);
        safe_write(tarfile, buf, 11);
        safe_write(tarfile, &nul, 1);
        free(buf);

        buf = (char *)malloc(sizeof(char) * 11);
        buf = decToOctal(mtime, buf, 11);
        safe_write(tarfile, buf, 11);
        safe_write(tarfile, &nul, 1);
        free(buf);

        safe_write(tarfile, spaces, 8);    // temporary checksum

        safe_write(tarfile, &typeflag, 1);

        safe_write(tarfile, &linkname, 100);

        safe_write(tarfile, MAGIC, 6);

        safe_write(tarfile, VERSION, 2);

        safe_write(tarfile, uname, 32);

        safe_write(tarfile, gname, 32);

        safe_write(tarfile, zeroes, 16);    // devmajor & devminor

        safe_write(tarfile, prefix, 155);

        safe_write(tarfile, zeroes, 12);

        lseek(tarfile, 0, SEEK_SET);
        buf = (char *)malloc(sizeof(char) * BLKSIZE);
        if (read(tarfile, buf, BLKSIZE) == -1)
            sys_error("read");
        chksum_ptr = &buf[0];
        for (j = 0; j < BLKSIZE; j++) {
            chksum += *chksum_ptr;
            chksum_ptr++;
        }
        free(buf);
        buf = (char *)malloc(sizeof(char) * 7);
        buf = decToOctal(chksum, buf, 7);
        lseek(tarfile, 148, SEEK_SET);
        safe_write(tarfile, buf, 7);
        safe_write(tarfile, &nul, 1);
        free(buf);

        lseek(tarfile, 0, SEEK_END);
        if (S_ISREG(st.st_mode)) {
            if ((file = open(argv[i], O_RDONLY)) == -1)
                sys_error("open");
            buf = (char *)calloc(BLKSIZE, sizeof(char));
            while ((n = read(file, buf, BLKSIZE)) > 0) {    // check read for system error
                if (n < BLKSIZE) {
                    for (j = n; j < BLKSIZE; j++) {
                        buf[j] = '\0';
                    }
                write(tarfile, buf, BLKSIZE);
                }
            }
            free(buf);
            buf = (char *)calloc(BLKSIZE, sizeof(char));
            for (j = 0; j < 2; j++) {
                /* Stop blocks */
                write(tarfile, buf, BLKSIZE);
            }
            close(file);
        }
    }
    close(tarfile);
}

int main(int argc, char **argv) {
        /* Comment */
        int flags[6] = {0};     /* Order: [c, t, x, v, S, f] */

        parse_cmd(argc, argv, flags);
        if (flags[0] == 1)
                tar_create(argc, argv, flags);
        else if (flags[1] == 1)
            tar_list(argv, flags);
        else if (flags[2] == 1)
            tar_extract(argv, flags);

        return 0;
}