#include "kernel/types.h"
#include "user/user.h"

#define READ 0
#define WRITE 1
#define BUFLEN 2

int main(int argc, char *argv[]){
	int ping[2];
	int pong[2];
	int pid;
	int cnt;
	char buf[2];

	
	if(pipe(ping) < 0){
		fprintf(2, "Pipe failed!\n");
		exit(1);
	}

	if(pipe(pong) < 0){
		fprintf(2, "Pipe failed!\n");
		exit(1);
	}

	if((pid = fork()) < 0){
		fprintf(2, "Fork failed!\n");
		exit(1);
	}


	if(pid != 0){ 	//parent
		close(ping[READ]);
		close(pong[WRITE]);

		pid = getpid();
		//send ping
		strcpy(buf, "c");
		write(ping[WRITE], buf, BUFLEN);
		//recv pong
		cnt = read(pong[READ], buf, BUFLEN);
		//check
		if(cnt != BUFLEN || strcmp(buf, "d") != 0){
			fprintf(2, "%d: read error!", pid);
		}
		printf("%d: received pong\n", pid);

		close(pong[READ]);
		close(ping[WRITE]);
	}
	else{ 	//child
		close(pong[READ]);
		close(ping[WRITE]);

		pid = getpid();

		//recv ping
		cnt = read(ping[READ], buf, BUFLEN);
		//check
		if(cnt != BUFLEN || strcmp(buf, "c") != 0){
			fprintf(2, "%d: read error!", pid);
		}
		printf("%d: received ping\n", pid);
		//send pong
		strcpy(buf, "d");
		write(pong[WRITE], buf, BUFLEN);

		close(ping[READ]);
		close(pong[WRITE]);
	}
	exit(0);
}
