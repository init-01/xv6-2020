#include "kernel/types.h"
#include "user/user.h"

#define READ 0
#define WRITE 1

#define STDIN 0
#define STDOUT 1

#define START 2
#define END 35

#define RSIZE sizeof(int)

void child_job(int input_pipe[]){
    close(input_pipe[WRITE]);
    int p;
    int n;
    int pid;
    int output_pipe[2];
    if(read(input_pipe[READ], &p, RSIZE) != 0){
        pipe(output_pipe);
        if((pid = fork()) != 0){    //parent
            printf("prime %d\n", p);
            while(read(input_pipe[READ], &n, RSIZE) != 0){
                if(n % p != 0){
                    write(output_pipe[WRITE], &n, RSIZE);
                }
            }
            close(input_pipe[READ]);
            close(output_pipe[READ]);
            close(output_pipe[WRITE]);
            wait(0);
            exit(0);
        }
        else{
            child_job(output_pipe);
        }
    }
}


int main(int argc, char *argv[]){
    int feedNumber[2];
    int pid;
    int i;

    if(pipe(feedNumber) < 0){
		fprintf(2, "Pipe failed!\n");
		exit(1);
	}

	if((pid = fork()) < 0){
		fprintf(2, "Fork failed!\n");
		exit(1);
	}

    if(pid != 0){   //feeder
        close(feedNumber[READ]);
        for(i = START; i <= END; i++){
            write(feedNumber[WRITE], &i, sizeof(int));
        }
        close(feedNumber[WRITE]);
    }
    else{           //child
        for(i = START; i <= END; i++){
            child_job(feedNumber);
        }
    }
    close(feedNumber[READ]);
    close(feedNumber[WRITE]);
    wait(0);
    exit(0);
}