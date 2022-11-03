#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#define BLOCK 512;

int main(int argc, char *argv[]){
    int i;
    int arg2len;
    int mode;
    int v;
    int S;
    int file;

    char *modes;
    if(argc <= 2){
        printf("1. usage mytar [ctxvS]f tarfile [ path [ ... ] ]");
        exit(EXIT_FAILURE);
    }
    modes = argv[1];
    arg2len = strlen(argv[1]) -1;
    if(modes[arg2len] != 'f'){
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

    if(mode == 'c'){
        file = open(argv[2], O_RDWR | O_CREAT | O_TRUNC);
        create_mode(file, v, argv[3]);
    }

    if(mode == 't'){
        file = open(argv[2], O_RDONLY);
        table_mode(file, v, argv[3]);
    }

     if(mode == 'x'){
        file = open(argv[2], O_RDONLY);
        extract_mode(file, v, argv[3]);
    }

}

int create_mode(int file, int v, char * args){

}

int table_mode(int file, int v, char * args){

}

int extract_mode(int file, int v, char * args){

}
