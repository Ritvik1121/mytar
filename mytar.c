#include "headers.h"


void parse_cmd(int argc, char **argv, int flags[6]) {
    /* Parses the command line arguments
        * Checks for user usage error, sets flags for program */
    int i;

    if (argc < 3) {
        fprintf(stderr, "usage: %s [ctxvS]f tarfile"
        "[ path [ ... ] ]\n", argv[0]);
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
            fprintf(stderr, "usage: %s [ctxvS]f tarfile"
            "[ path [ ... ] ]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    if (flags[0] != 1 && flags[1] != 1 && flags[2] != 1 && flags[5] != 1) {
        /* Flag c, t, x, or f are necessary */
        fprintf(stderr, "%s: you must specify at least one"
        "of the 'ctx' options.", argv[0]);
        fprintf(stderr, "usage: %s [ctxvS]f"
        "tarfile [ path [ ... ] ]\n", argv[0]);
        exit(EXIT_FAILURE);
    } else if ((flags[0] == 1 && (flags[1] == 1 || flags[2] == 1)) ||
                (flags[1] == 1 && (flags[0] == 1 || flags[2] == 1)) ||
                (flags[2] == 1 && (flags[0] == 1 || flags[1] == 1))) {
        /* Flags c, t, and x cannot be used together */
        fprintf(stderr, "%s: you must choose one of"
        "the 'ctx' options.", argv[0]);
        fprintf(stderr, "usage: %s [ctxvS]f tarfile"
        "[ path [ ... ] ]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
}


int main(int argc, char **argv) {
        /* Comment */
        int flags[6] = {0};     /* Order: [c, t, x, v, S, f] */
        int file;


        parse_cmd(argc, argv, flags);
        if (flags[0] == 1) {
            createTar(argc, argv, flags);
        }

        else if (flags[1] == 1) {
            if((file = open(argv[2], O_RDONLY)) == -1){
                perror("open");
                exit(EXIT_FAILURE);
            }
            table_mode(file, flags, argc-3, argv);
            if((close(file))==-1){
                perror("close");
                exit(EXIT_FAILURE);
            }
        }

        else if (flags[2] == 1) {
            if((file = open(argv[2], O_RDONLY)) == -1){
                perror("open");
                exit(EXIT_FAILURE);
            }
            extractTar(file, argc, argv, flags);
            if((close(file))==-1){
                perror("close");
                exit(EXIT_FAILURE);
            }
        }

        return 0;
}
