#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include "extract.c"
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

    if(mode == 't') {
        file = open(argv[2], O_RDONLY);
        table_mode(file, v, arguments);
    }

    
    if(mode == 'x'){
        file = open(argv[2], O_RDONLY);
        extractTar(file, v, arguments);
    }
    return 0;
}

int table_mode(int file, int v, char **args){
    unsigned char buffer[512];
    int n;
    int i = 0;
    int num;
    int k;
    char name[200];
    char size[12];
    int num_blocks;
    int new_index;

    while((n = read(file, buffer, 100)) > 0){

        i = 0;

        while(i < 100 || buffer[i] == '\0'){
            name[i] = buffer[i];
            i++;
        }

        if(name[0] == '\0'){
            return 0;
        }

        name[i] = '\0';
        printf("%s\n", name);

        lseek(file, 23, SEEK_CUR);
        k = read(file, size, 12);
        num = octaltodec(size, 12);

        if(num == 0){
            num_blocks = 0;
        }
        num_blocks = (num / 512) + 1;
        new_index = 377 + (512 * num_blocks);
        lseek(file, new_index, SEEK_CUR);
    }



}

int octaltodec(char *number, int len){
    int i;
    int curr;
    int power = 0;
    int total = 0;
    for(i = len -1; i > 0; i--){

        curr = number[i] - '0';
        total = total + (curr * (pow(8, power)));
        power++;
    }
    return total;
}