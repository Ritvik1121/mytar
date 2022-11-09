#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#define BLOCK 512;

char *find_permissions(long permissions, char *type);

int table_mode(int file, int v, char **args, int arg){
    char buffer[512] = {0};
    int n;
    int i = 0;
    long num;
    int k;
    char name[256] = {0};
    char prefix[155]= {0};
    char size[12]= {0};
    int num_blocks;
    int new_index;
    char *permissions;
    char mode[8]= {0};
    char mtime[12]= {0};
    char uid[8]= {0};
    char gid[8]= {0};
    char uname[32] = {0};
    char gname[32] = {0};
    char checksum[8] = {0};
    char typeflag[1] = {0};
    char outstr[20]= {0};

    struct tm *tmp;
    long fmode, ftime;



    while((n = read(file, buffer, 100)) > 0){

        char *ptr;
        /*Get the size*/
        k = read(file, mode, 8);

        k = read(file, uid, 8);
        k = read(file, gid, 8);

        k = read(file, size, 12);
        num = strtol(size, &ptr, 8);

        k = read(file, mtime, 12);
        ftime = strtoul(mtime, &ptr, 8);
        tmp = localtime(&ftime);
        strftime(outstr, sizeof(outstr), "%F %H:%M", tmp);

        k = read(file, checksum, 8);
        k = read(file, typeflag, 1);
        lseek(file, 108, SEEK_CUR);
        k = read(file, uname, 32);
        k = read(file, gname, 32);


        /*Get the prefix*/
        lseek(file, 16, SEEK_CUR);
        k = read(file, prefix, 155);

        /*Add prefix if there is something there*/
        i = 0;
        if(prefix[0] != '\0'){
            strcpy(name, prefix);
            strcat(name, "/");
            strcat(name, buffer);
        }
        else {
            strcpy(name, buffer);
        }

        /*check if the name is null if so then stop*/
        if(name[0] == '\0'){
            return 0;
        }

        /*If no arguments then print or else only print descendents*/
        if(arg == 0){
            if(v == 1){

                fmode = strtoul(mode, &ptr, 8);
                permissions = find_permissions(fmode, typeflag);
                printf("%s ", permissions);

                printf("%s/%s ", uname, gname);

                printf("%14ld ", num);
                printf("%s ", outstr); /*time*/

            }
            printf("%s\n", name);
        }
        else{

            for(i = 0; i < arg; i++){
                if(strstr(name, args[i]) != NULL){
                    if(v == 1){
                        fmode = strtoul(mode, &ptr, 8);
                        permissions = find_permissions(fmode, typeflag);
                        printf("%s ", permissions);

                        printf("%s/%s ", uname, gname);

                       /*size */
                        printf("%14ld ", num);
                        printf("%s ", outstr); /*time*/

                    }
                    printf("%s\n", name);
                }
            }
        }

        /*Get offset to go to next header then lseek*/

        if(num == 0){
            num_blocks = 0;
        }
        else {
            num_blocks = (num / 512) + 1;
        }

        new_index = 12 + (512 * num_blocks);
        lseek(file, new_index, SEEK_CUR);

    }
    return 0;
}

char *find_permissions(long permissions, char *type){
    char permission[11];

    if(type[0] == '2'){
        permission[0] = 'l';
    }
    else if(type[0] == '5'){
        permission[0] = 'd';
    }
    else {
        permission[0] = '-';
    }

    if(permissions & S_IRUSR){
        permission[1] = 'r';
    }
    else {
        permission[1] = '-';
    }
    if(permissions & S_IWUSR){
        permission[2] = 'w';
    }
    else {
        permission[2] = '-';
    }
    if(permissions & S_IXUSR){
        permission[3] = 'x';
    }
    else {
        permission[3] = '-';
    }
    if(permissions & S_IRGRP){
        permission[4] = 'r';
    }
    else {
        permission[4] = '-';
    }
    if(permissions & S_IWGRP){
        permission[5] = 'w';
    }
    else {
        permission[5] = '-';
    }
    if(permissions & S_IXGRP){
        permission[6] = 'x';
    }
    else {
        permission[6] = '-';
    }
    if(permissions & S_IROTH){
        permission[7] = 'r';
    }
    else {
        permission[7] = '-';
    }
    if(permissions & S_IWOTH){
        permission[8] = 'w';
    }
    else {
        permission[8] = '-';
    }
    if(permissions & S_IXOTH){
        permission[9] = 'x';
    }
    else {
        permission[9] = '-';
    }
    permission[10] = '\0';

    return strdup(permission);
}
