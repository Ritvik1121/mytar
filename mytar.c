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

#define NAME_LEN 100
#define MODE_LEN 8
#define UID_LEN 8
#define GID_LEN 8
#define SIZE_LEN 12
#define MTIME_LEN 12
#define CHKSUM_LEN 8
#define TYPEFLAG_LEN 1
#define LINKNAME_LEN 100
#define MAGIC_LEN 6
#define VERSION_LEN 2
#define UNAME_LEN 32
#define GNAME_LEN 32
#define DEVMAJOR_LEN 8
#define DEVMINOR_LEN 8
#define PREFIX_LEN 155
#define POST_HEADER_LEN 12

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

char *decToOctal(int num, char *buffer, int length) {
    /* Converts decimal num into base octal, stores as string into buffer 
     * If num is too large to represent, uses insert_special_int() */
    int i, orig_num = num;

    for (i = length - 1; num != 0; i--) {
        if (i < 0) {
            /* num cannot fit in [length] octal digits*/
            if (insert_special_int(buffer, length + 1, orig_num) != 0)
                sys_error("number too large");
            return buffer;
        }
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

unsigned int checksum(char *buffer, int length) {
    /* Calculates checksum from given buffer */
    unsigned int chksum = 0, i;
    unsigned int *chksum_ptr;

    chksum_ptr = &buffer[0];
    for (i = 0; i < length; i++) {
        chksum += *chksum_ptr;
        chksum_ptr++;
    }
    return chksum;
}

void tar_create(int argc, char **argv, int flags[6]) {
    /* Create file for tar */
    unsigned int i, j, n, namelen, tarfile, file, chksum = 0;
    struct stat st;
    struct passwd *pwd;
    struct group *grp;
    char spaces[CHKSUM_LEN] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    int zeroes[DEVMAJOR_LEN + DEVMINOR_LEN] = {0};
    char name[NAME_LEN] = {'\0'};
    char prefix[PREFIX_LEN] = {'\0'};
    char linkname[LINKNAME_LEN] = {'\0'};
    char uname[UNAME_LEN] = {'\0'}, gname[GNAME_LEN] = {'\0'};  /* NULL-terminated */
    char *buf;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    off_t size;
    time_t mtime;
    char typeflag, nul = '\0';

    tarfile = open(argv[2], O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
    for (i = 3; i < argc; i++) {
        /* Loop through path(s) */
        if (lstat(argv[i], &st) == -1)
            sys_error("lstat");

        for (namelen = 0; namelen < strlen(argv[i]) && namelen < NAME_LEN; namelen++) {
            /* Stores first 100 characters or less of name */
            name[namelen] = argv[i][namelen];
        }
        if (namelen == 100) {
            /* If name is over 100 chars, put rest in prefix */
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
            if (readlink(argv[i], linkname, sizeof(linkname)) == -1)
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

        safe_write(tarfile, name, NAME_LEN);

        buf = (char *)malloc(sizeof(char) * (MODE_LEN - 1));
        mode &= 0xFFF;  /* Only care about last 12 bits for permission */
        buf = decToOctal(mode, buf, MODE_LEN - 1);
        safe_write(tarfile, buf, MODE_LEN - 1);
        safe_write(tarfile, &nul, 1);
        free(buf);

        buf = (char *)malloc(sizeof(char) * UID_LEN);
        buf = decToOctal(uid, buf, UID_LEN - 1);    /* make sure to check if uid was long enough or not */
        safe_write(tarfile, buf, UID_LEN);
        free(buf);

        buf = (char *)malloc(sizeof(char) * (GID_LEN - 1));
        buf = decToOctal(gid, buf, (GID_LEN - 1));
        safe_write(tarfile, buf, (GID_LEN - 1));
        safe_write(tarfile, &nul, 1);
        free(buf);

        buf = (char *)malloc(sizeof(char) * (SIZE_LEN - 1));
        buf = decToOctal(size, buf, SIZE_LEN - 1);
        safe_write(tarfile, buf, SIZE_LEN - 1);
        safe_write(tarfile, &nul, 1);
        free(buf);

        buf = (char *)malloc(sizeof(char) * (MTIME_LEN - 1));
        buf = decToOctal(mtime, buf, MTIME_LEN - 1);
        safe_write(tarfile, buf, MTIME_LEN - 1);
        safe_write(tarfile, &nul, 1);
        free(buf);

        safe_write(tarfile, spaces, CHKSUM_LEN);    /* temporary checksum */

        safe_write(tarfile, &typeflag, TYPEFLAG_LEN);

        safe_write(tarfile, &linkname, LINKNAME_LEN);

        safe_write(tarfile, MAGIC, MAGIC_LEN);

        safe_write(tarfile, VERSION, VERSION_LEN);

        safe_write(tarfile, uname, UNAME_LEN);

        safe_write(tarfile, gname, GNAME_LEN);

        safe_write(tarfile, zeroes, DEVMAJOR_LEN + DEVMINOR_LEN);

        safe_write(tarfile, prefix, PREFIX_LEN);

        safe_write(tarfile, zeroes, POST_HEADER_LEN);

        lseek(tarfile, 0, SEEK_SET);
        buf = (char *)malloc(sizeof(char) * BLKSIZE);
        if (read(tarfile, buf, BLKSIZE) == -1)
            sys_error("read");
        chksum = checksum(buf, BLKSIZE);
        free(buf);
        buf = (char *)malloc(sizeof(char) * (CHKSUM_LEN - 1));
        buf = decToOctal(chksum, buf, CHKSUM_LEN - 1);
        lseek(tarfile, CHKSUM_OFFSET, SEEK_SET);
        safe_write(tarfile, buf, (CHKSUM_LEN - 1));
        safe_write(tarfile, &nul, 1);
        free(buf);

        lseek(tarfile, 0, SEEK_END);
        if (S_ISREG(st.st_mode)) {
            if ((file = open(argv[i], O_RDONLY)) == -1)
                sys_error("open");
            buf = (char *)calloc(BLKSIZE, sizeof(char));
            while ((n = read(file, buf, BLKSIZE)) > 0) {    /* check read for system error */
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
            free(buf);
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