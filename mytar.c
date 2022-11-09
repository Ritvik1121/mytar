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



int table_mode(int file, int v, char **args, int argc);
char *find_permissions(long permissions, char *type);

u_int32_t extract_special_int (char *where, int len) {
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

int main(int argc, char *argv[]){
    int i;
    int arg2len;
    int mode;
    int v;
    int file;
    int j = 0;
    int count = 0;


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

    }
    for(i = 3; i < argc; i++){
        arguments[j] = argv[i];
        j++;
        count++;
    }

    if(mode == 't'){
        file = open(argv[2], O_RDONLY);
        table_mode(file, v, arguments, count);
    }
    return 0;


}
