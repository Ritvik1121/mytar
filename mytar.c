#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#define BLOCK 512;

int octaltodec(char *number, int len);
int table_mode(int file, int v, char **args, int argc);

int main(int argc, char *argv[]){
    int i;
    int arg2len;
    int mode;
    int v;
    int S;
    int file;

    char **arguments = (char **) malloc(10 * sizeof(char *));

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
    int count;

    for(i = 3; i < argc; i++){
        arguments[j] = argv[i];
        j++;
        count++;
    }

    if(mode == 't'){
        file = open(argv[2], O_RDONLY);
        table_mode(file, v, arguments, count);
    }


}



int table_mode(int file, int v, char **args, int arg){
    unsigned char buffer[512];
    int n;
    int i = 0;
    int num;
    int k;
    char name[256];
    char prefix[155];
    char size[12];
    int num_blocks;
    int new_index;
    int len;

    while((n = read(file, buffer, 100)) > 0){

        /*Get the size*/
        lseek(file, 23, SEEK_CUR);
        k = read(file, size, 12);
        num = octaltodec(size, 12);

        /*Get the prefix*/
        lseek(file, 210, SEEK_CUR);
        k = read(file, prefix, 155);

        /*Add prefix if there is something there*/
        i = 0;
        if(prefix[0] != '\0'){
            printf("here");
            while(i < 155 || prefix[i] != '\0'){
                name[i] = prefix[i];
                i++;
            }
        }

        /*Add the name field*/
        while(i < 100 || buffer[i] != '\0'){
            name[i] = buffer[i];
            i++;
        }

        /*check if the name is null if so then stop*/
        if(name[0] == '\0'){
            return 0;
        }

        /*If no arguments then print or else only print descendents*/
        if(arg == 0){
            printf("%s\n", name);
        }
        else{
            for(i = 0; i < arg; i++){
                if(strstr(name, args[i]) != NULL){
                    printf("%s\n", name);
                }
            }
        }

        /*Get offset to go to next header then lseek*/
        num_blocks = (num / 512) + 1;
        if(num == 0){
            num_blocks = 0;
        }
        new_index = 12 + (512 * num_blocks);
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
