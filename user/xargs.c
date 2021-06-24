#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

#define MAXBUF 512

#ifndef NULL
#define NULL 0
#endif

int main(int argc, char *argv[]){

    char buf[MAXBUF];
    char *_argv[MAXARG];

    for(int i = 1; i < argc; i++){
        _argv[i-1] = argv[i];
    }
    for(int i = argc-1; i < MAXARG; i++){
        _argv[i] = NULL;
    }

    memset(buf, 0, MAXBUF);
    
    int readn = read(0, buf, MAXBUF-1);

    if(readn == MAXBUF-1){
        if(read(0, buf+MAXBUF-1, 1)){
            fprintf(2, "xargs: stdin too long! max available buffer size: %d", MAXBUF-1);
            exit(1);
        }
    }

    if(strlen(buf) != readn){
        fprintf(2, "xargs: WARNING: a NUL character occurred in the input.  It cannot be passed through in the argument list.");
    }

    char *ptr = buf;
    while(1){
        int _argc = argc - 1;
        int stop;
        char* end = strchr(ptr, '\n');

        if(end){
            stop = 0;
            *end = '\0';
        }
        else{
            stop = 1;
            end = ptr + strlen(ptr);
        }
        
        for(int i = argc-1; i < MAXARG; i++){
            _argv[i] = NULL;
        }

        //Split buf and fill _argv
        char* next;
        do{
            if((next = strchr(ptr, ' '))){
                *next = '\0';
            }
            if(*ptr){   //skip zero length string
                _argv[_argc++] = ptr;
            }
            ptr = next? next + 1 : end + 1; //if next is null -> set ptr to end of string
        }while(next);


        if(_argc == argc - 1){  //nothing from stdin -> skip
            if(stop){
                break;
            }
            else{
                continue;
            }
        }

        if(fork() == 0){
            exec(argv[1], _argv);
        }
        else{
            wait(0);
            if(stop){
                break;
            }
        }
    }

    exit(0);
}