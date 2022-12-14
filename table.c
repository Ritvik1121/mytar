#include "headers.h"

int table_mode(int file, int *flags, int arg, char **args){

    char name[NAME_SIZE] = {0};
    char block[BLOCK];
    int n;
    int i = 0;
    long num;
    char path[PATH_SIZE+1] = {0};
    char prefix[PREFIX_SIZE]= {0};
    char size[SIZE_SIZE]= {0};
    int num_blocks;
    int new_index;
    char *permissions;
    char mode[MODE_SIZE]= {0};
    char mtime[MTIME_SIZE]= {0};
    char uid[UID_SIZE]= {0};
    char gid[GID_SIZE]= {0};
    char uname[UNAME_SIZE] = {0};
    char gname[GNAME_SIZE] = {0};
    char chksum[CHKSUM_SIZE] = {0};
    char typeflag[TYPEFLAG_SIZE] = {0};
    char outstr[OUTTIME_SIZE]= {0};

    char magic_number[MAGIC_SIZE] = {0};
    char version[VERSION_SIZE] = {0};

    unsigned int cs = 0;
    unsigned int check = 0;

    struct tm *tmp;
    long user, group, fmode, ftime;


    while((!(check_end(file)))){

        /*read in the first byte which is the header */
        char *ptr;

        n = read(file, block, BLOCK);
        if(n == -1){
            perror("Reading failed");
            exit(EXIT_FAILURE);
        }

	/*read in all the necessary fields*/
        for (i = 0; i < NAME_SIZE; i++) {
            name[i] = block[i];
        }

        for (i = 0; i < MODE_SIZE; i++) {
            mode[i] = block[MODE_OFFSET+ i];
        }

        for (i = 0; i < UID_SIZE; i++) {
            uid[i] = block[UID_OFFSET + i];
        }
        user = strtoul(uid, &ptr, BASED);
        if (user == ULONG_MAX && errno == ERANGE)  {
            fprintf(stderr, "stroul overflow\n");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i < GID_SIZE; i++) {
            gid[i] = block[GID_OFFSET + i];
        }
        group = strtoul(gid, &ptr, BASED);
        if (group == ULONG_MAX && errno == ERANGE)  {
            fprintf(stderr, "stroul overflow\n");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i < SIZE_SIZE; i++) {
            size[i] = block[SIZE_OFFSET + i];
        }
        num = strtoul(size, &ptr, BASED);
        if (num == ULONG_MAX && errno == ERANGE)  {
            fprintf(stderr, "stroul overflow\n");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i < MTIME_SIZE; i++) {
            mtime[i] = block[MTIME_OFFSET + i];
        }

        ftime = strtoul(mtime, &ptr, BASED);
        if (ftime == ULONG_MAX && errno == ERANGE)  {
            fprintf(stderr, "stroul overflow\n");
            exit(EXIT_FAILURE);
        }

	/*change the time to an actual time*/
        tmp = localtime(&ftime);
        if(tmp == NULL){
            perror("localtime failed");
            exit(EXIT_FAILURE);
        }
        strftime(outstr, sizeof(outstr), "%Y-%m-%d %H:%M", tmp);

        for (i = 0; i < CHKSUM_SIZE; i++) {
            chksum[i] = block[CHKSUM_OFFSET + i];
        }

        for (i = 0; i < MAGIC_SIZE; i++) {
            magic_number[i] = block[MAGIC_OFFSET + i];
        }

        for (i = 0; i < VERSION_SIZE; i++) {
            version[i] = block[VERSION_OFFSET + i];
        }


       for (i = 0; i < TYPEFLAG_SIZE; i++) {
            typeflag[i] = block[TYPEFLAG_OFFSET + i];
        }

        for (i = 0; i < UNAME_SIZE; i++) {
            uname[i] = block[UNAME_OFFSET + i];
        }

        for (i = 0; i < GNAME_SIZE; i++) {
            gname[i] = block[GNAME_OFFSET + i];
        }

        /*Get the prefix*/
       for (i = 0; i < PREFIX_SIZE; i++) {
            prefix[i] = block[PREFIX_OFFSET + i];
        }

	/*Checks if ustar is right */
        if (strncmp("ustar", magic_number, MAGIC_SIZE-1) != 0) {
            fprintf(stderr, "bad magic number\n");
            exit(EXIT_FAILURE);
        }

        if (flags[STRICT] == 1) {
	    /*Strict mode checks*/
            if (magic_number[strlen(magic_number)] != '\0') {
                fprintf(stderr, "bad magic number\n");
                exit(EXIT_FAILURE);
            }


            if (strncmp(version, VERSION, 2) != 0) {
                fprintf(stderr, "bad version\n");
                exit(EXIT_FAILURE);
            }
        }

        /*Add prefix if there is something there*/
        i = 0;
        if(prefix[0] != '\0'){
            strcpy(path, prefix);
            strcat(path, "/");
            strcat(path, name);
        }
        else {
            strcpy(path, name);
        }

	/*Checks if the header is invalid*/
        cs = checksum(block, BLOCK);
        check = strtoul(chksum, &ptr, BASED);
        if (check == ULONG_MAX && errno == ERANGE)  {
            fprintf(stderr, "stroul overflow\n");
            exit(EXIT_FAILURE);
        }
        if(cs != check){
            fprintf(stderr, "Corrupted header\n");
            exit(EXIT_FAILURE);
        }

        /*If no arguments then print or else only print descendents*/

        if(arg == 0){

            if(flags[VERBOSE] == 1){

                fmode = strtoul(mode, &ptr, BASED);
                if (fmode == ULONG_MAX && errno == ERANGE)  {
                    fprintf(stderr, "stroul overflow\n");
                    exit(EXIT_FAILURE);
                }
                permissions = find_permissions(fmode, typeflag);
                printf("%s ", permissions);
                free(permissions);

		/*check if symbolic names are present*/
                if(strlen(uname) == 0){
                    printf("%ld", user);
                }
                else {
                   printf("%s", uname);
                }
                printf("/");

                if(strlen(gname) == 0){
                    printf("%ld", group);
                }
                else {
                   printf("%s ", gname);
                }

                printf("%14ld ", num);
                printf("%s ", outstr); /*time*/

            }
            printf("%s\n", path);
        }
        else{
            for(i = 3; i < arg+3; i++){
                int len = strlen(args[i]);
                if(strncmp(path, args[i], len) == 0){
                    /*This loop is for named files*/
                    if(flags[3] == 1){
                        fmode = strtoul(mode, &ptr, BASED);
                        if (fmode == ULONG_MAX && errno == ERANGE)  {
                            fprintf(stderr, "stroul overflow\n");
                            exit(EXIT_FAILURE);
                        }
                        permissions = find_permissions(fmode, typeflag);
                        printf("%s ", permissions);
                        free(permissions);

                        if(strlen(uname) == 0){
                            printf("%ld", user);
                        }
                        else {
                            printf("%s", uname);
                        }
                        printf("/");

                        if(strlen(gname) == 0){
                            printf("%ld", group);
                        }
                        else {
                            printf("%s ", gname);
                        }
                       /*size */
                        printf("%14ld ", num);
                        printf("%s ", outstr); /*time*/

                    }
                    printf("%s\n", path);
                }
            }
        }

        /*Get offset to go to next header then lseek*/
        if(num == 0){
            num_blocks = 0;
        }
        else {
            num_blocks = (num / BLOCK) + 1;
        }

        new_index = BLOCK * num_blocks;
        if((lseek(file, new_index, SEEK_CUR)) == -1){
            perror("lseek failed");
            exit(EXIT_FAILURE);
        }



    }
    return 0;
}

char *find_permissions(long permissions, char *type){
	/*gets the permissions and the type of file*/
    char permission[PERMLEN];

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
