#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void signal_handler(int signal){
    if (signal == SIGINT){puts("can't stop me!");
    }
}

int main(){
    if ((signal(SIGINT, signal_handler)) == SIG_ERR){
	puts("error connecting signal handler");
	exit(1);
    }
    while (true){sleep(1);
    }
    return 0;
}
