#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#define BLOCK 512;

int octaltodec(char *number, int len);
int table_mode(int file, int v, char **args);

int main(int argc, char *argv[]){
    int i;
    int arg2len;
    int mode;
    int v;
    int S;
    int file;

    char **arguments = (char **) malloc(10 * sizeof(char *));
    int table_mode(int file, int v, char **args);

    char *modes;
    if(argc <= 2){
        printf("1. usage mytar [ctxvS]f tarfile [ path [ ... ] ]");
        exit(EXIT_FAILURE);
    }
    modes = argv[1];
    arg2len = strlen(argv[1]) -1;
    if(modes[arg2len] != 'f'){
        printf("%c\n", modes[arg2len]);
        printf("2. usage mytar [ctxvS]f tarfile [ path [ ... ] ]");
        exit(EXIT_FAILURE);
    }

    for(i = 0; i < arg2len; i++){
        if(modes[i] == 'c')
            mode = 'c';

        if(modes[i] == 't')
            mode = 't';

        if(modes[i] == 'x')
            mode = 'x';

        if(modes[i] == 'v')
            v = 1;

        if(modes[i] == 'S')
            S = 1;

    }

    int j = 0;

    for(i = 3; i < argc; i++){
        arguments[j] = argv[i];
        j++;
    }

    if(mode == 't'){
        file = open(argv[2], O_RDONLY);
        table_mode(file, v, arguments);
    }


}