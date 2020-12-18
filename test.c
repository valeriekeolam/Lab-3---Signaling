#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>

struct count_lock{
        int sig_gotUSRone;
        int sig_gotUSRtwo;
        int sig_sentUSRtwo;
        int sig_sentUSRone;
        pthread_mutex_t mutex_sentUSRone;
        pthread_mutex_t mutex_sentUSRtwo;
        pthread_mutex_t mutex_gotUSRone;
        pthread_mutex_t mutex_gotUSRtwo;
        time_t timesum_sigone;
        time_t timesum_sigtwo;
};

void signal_handler(int signal_number){
        printf("In signal handler\n");
        time_t current_time;
        time_t time_since_last_sigtwo;
        time_t time_since_last_sigone;

        printf("signal_number is %d\n", signal_number);
        printf("SIGUSR2: %d\n", SIGUSR2);
        printf("SIGUSR1: %d\n", SIGUSR1);

        //receive signal, ignore the other
        if (signal_number == SIGUSR2){
                //Lock and then increase the counter for received SIGUSR2 signals.
//              pthread_mutex_lock(&(shm_ptr->mutex_gotUSRtwo));
//              shm_ptr->sig_gotUSRtwo++;
//              pthread_mutex_unlock(&(shm_ptr->mutex_gotUSRtwo));

                printf("SIGUSR2 RECEIVED %d\n", shm_ptr->sig_gotUSRtwo);
                                                                                                                                     //Collect the current time value. Note that start_time is not zero.
                current_time = time(NULL);
                current_time = current_time - start_time;

                //Add the time since the last SIGUSR2 signal to the timesum variable.                                                if(shm_ptr->sig_gotUSRtwo < 2){
                        time_since_last_sigtwo = current_time;                                                                               shm_ptr->timesum_sigtwo = current_time;
                } else {
                        shm_ptr->timesum_sigtwo = (shm_ptr->timesum_sigtwo + (current_time - time_since_last_sigtwo)>                        time_since_last_sigtwo = current_time;
                }
                                                                                                                             //receive signal, ignore the other                                                                                   } else if (signal_number == SIGUSR1){                                                                                        //Lock and then increase the counter for received SIGUSR1 signals.                                   //              pthread_mutex_lock(&(shm_ptr->mutex_gotUSRone));                                                     //              shm_ptr->sig_gotUSRone++;                                                                            //              pthread_mutex_unlock(&(shm_ptr->mutex_gotUSRone));

                printf("SIGUSR1 RECEIVED %d\n", shm_ptr->sig_gotUSRone);

                //Collect the current time value. Note that start_time is not zero.
                current_time = time(NULL);
                current_time = current_time - start_time;

                //Add the time since the last SIGUSR1 signal to the timesum variable.
                if(shm_ptr->sig_gotUSRone < 2){
                        time_since_last_sigone = current_time;
                        shm_ptr->timesum_sigone = current_time;
                } else {
                        shm_ptr->timesum_sigone = (shm_ptr->timesum_sigone + (current_time - time_since_last_sigone)>                        time_since_last_sigone = current_time;
                }
        }

}

int main(){
	kill()
}
