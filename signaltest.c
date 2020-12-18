#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void sig_handler(int signum) {

printf("Hola\n");
}

void sig_exit() {
printf("exiting from signal");
exit(0);
}

int main() {

int i = 1;
printf("Hello\n");
int pid;
if ( (pid = fork()) == 0) {
signal(SIGUSR1, sig_handler);
signal(SIGQUIT, sig_exit);
while(1) {
pause();
}
printf("EXITING!\n");
return 0;
}

sleep(1);

printf("STARTING!\n");
for (int i = 0; i < 10; i++) {
printf("%d sender\n", i);
if (kill(pid,SIGUSR1) < 0)
printf("didn't send\n");
sleep(1);

}
kill(pid,SIGQUIT);
printf("LEAVING!\n");
return 0;
}
