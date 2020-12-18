/*
 * Programmer: Valerie Lam
 * Class: CIS 3207 - Section 001
 * Due Date: November 11th, 2020
 * Assignment: Project 3 - Signaling with Multi-Process Programs
 * Version: 1
 * Description: Create a multi-process program that uses signals to interact.
 */

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

//NOTE: Structure is global, no need to explicitly pass it into functions.
//The purpose of this structure is to organize the counters, for SIGUSR1 & SIGUSR2, as well as locks.
struct count_lock{
	int sig_gotUSRone;
	int sig_gotUSRtwo;
	int sig_sentUSRtwo;
	int sig_sentUSRone;
	int reportCount;
	pthread_mutex_t mutex_sentUSRone;
	pthread_mutex_t mutex_sentUSRtwo;
	pthread_mutex_t mutex_gotUSRone;
	pthread_mutex_t mutex_gotUSRtwo;
	pthread_mutex_t mutex_reportCount;
	time_t timesum_sigone;
	time_t timesum_sigtwo;

};

#define DELAY_MIN 10 //.01 sec, 10 milliseconds
#define DELAY_MAX 100 //.1 sec, 100 milliseconds
#define GEN_MIN 1
#define GEN_MAX 10

void parent_process();
void block_signal(int signal_number);
void signal_generator();
void signal_handler(int signal_number);
void report(int signal_number);

time_t start_time;
struct count_lock *shm_ptr;

int main(){

	printf("Beginning Multi-Process Program\n");
	int sharedmem_id; //Original parent PID that is being given to all child processes.
//	struct count_lock *shm_ptr;
	pthread_mutexattr_t attr;
	srand(time(NULL) + getpid());

	//To get the start time in seconds.
	start_time = time(NULL);

	//Create a shared memory region.
	sharedmem_id = shmget(IPC_PRIVATE, sizeof(struct count_lock), IPC_CREAT | 0666);

	//Check memory creation for errors.
	assert(sharedmem_id >= 0);

	//Attach the shared memory to the structure pointer.
	shm_ptr = (struct count_lock *) shmat(sharedmem_id, NULL, 0);

	//Check memory creation for errors..
	assert(shm_ptr != (struct count_lock *) -1);

	//Initialize structure signal counters for SIGUSR1 & SIGUSR2.
	shm_ptr->sig_gotUSRone = 0;
	shm_ptr->sig_gotUSRtwo = 0;
	shm_ptr->sig_sentUSRone = 0;
	shm_ptr->sig_sentUSRtwo = 0;
	shm_ptr->reportCount = 0;

	//Initialize structure lock counters for SIGUSR1 & SIGUSR2.
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&(shm_ptr->mutex_sentUSRone), &attr);
	pthread_mutex_init(&(shm_ptr->mutex_sentUSRtwo), &attr);
	pthread_mutex_init(&(shm_ptr->mutex_gotUSRone), &attr);
	pthread_mutex_init(&(shm_ptr->mutex_gotUSRtwo), &attr);
	pthread_mutex_init(&(shm_ptr->mutex_reportCount), &attr);

	parent_process();

	//Detach shared memory.
//	shmdt(shm_ptr);
	return EXIT_SUCCESS;
}


void parent_process(){
	int counter = 0, i;
	pid_t pid;

	//The purpose of this is due to kill() PID variable being zero. This means that the signals will be sent
	//to all processes within the same process group (all nine processes, including parent). Thus, we must
	//also block the signal from also being received in the parent process.
//	block_signal(SIGUSR1);
//	block_signal(SIGUSR2);

//	printf("SIGUSR1: %d\nSIGUSR2: %d\n", SIGUSR1, SIGUSR2);

	for(i = 0; i < 8; i++){
//		printf("In forking loop\n");
                pid = fork();
                if(pid == 0){   //If pid is 0, meaning, if in the child process.
                         if(i < 2){ //Create signal handlers for SIGUSR1
                                //block SIGUSR2 using signal masking
//				printf("The value of i is %d\n", i);
				block_signal(SIGUSR2);

				//Does the following line of code start the process or "make up a signal"?
				//The following statement makes it so that the signal SIGUSR1 will activate
				//the signal handler whenever it is in encountered, even when the process
				//appears to be sleeping.
                                if(signal(SIGUSR1, signal_handler) == SIG_ERR){
					puts("Error connecting to signal handler.");
					exit(1);
				}

                                //The purpose of this is to prevent the function from terminating indep.
				while(1){
					sleep(1);
				}
                         } else if (i < 4){ //Signal handlers for SIGUSR2
                                //block sigusr1 using signal masking
				block_signal(SIGUSR1);

				if(signal(SIGUSR2, signal_handler) == SIG_ERR){
					puts("Error connecting to signal handler.");
					exit(1);
				}

				//The purpose of this is to prevent the function from terminating indep.
				while(1){
					sleep(1);
                                }
                         } else if (i < 7){ //Create three signal generators
                                //block both SIGUSR1 and SIGUSR2, no need for signal_generator to get sigs
				block_signal(SIGUSR1);
				block_signal(SIGUSR2);
                                signal_generator();
                         } else { //Create reporting process
				if((signal(SIGUSR1, report) == SIG_ERR) || (signal(SIGUSR2, report) == SIG_ERR)){
				puts("Error connecting to report process.");
					exit(1);
				}
				//The purpose of this is to prevent the function from terminating indep.
                                while(1){
                                        sleep(1);
                                }
			}
		}
	}
}

void block_signal(int signal_number){
	sigset_t signal_set;

	//Empty the set.
	sigemptyset(&signal_set);

	//Check which signal was received.
	if(signal_number == SIGUSR1){
		sigaddset(&signal_set, SIGUSR1);
	} else if(signal_number == SIGUSR2){
		sigaddset(&signal_set, SIGUSR2);
	}

	//This line blocks unwanted signals.
	sigprocmask(SIG_BLOCK, &signal_set, NULL);
}

/*The purpose of this function is to "handle the signals". In the lab, it is stated that there must be two
 *handlers per signal type (SIGUSR1 or SIGUSR2), and that those handlers ignore signals that they are not
 *designated to handle. The "ignore" is done by signal masking. Refer to function block_signal.
 *
 *What this function does is it is activated by a signal, it's corresponding signal, and then it checks for
 *the matching signal response. It then "locks" or prevents any other process from accessing the shared
 *memory structure at the same time and modifying the variable. The handler then increments the corresponding
 *count variable and then unlocks.
 */
void signal_handler(int signal_number){
	time_t current_time;
	time_t time_since_last_sigtwo;
	time_t time_since_last_sigone;

//	printf("signal_number is %d\n", signal_number);
//	printf("SIGUSR2: %d\n", SIGUSR2);
//	printf("SIGUSR1: %d\n", SIGUSR1);

	//receive signal, ignore the other
	if (signal_number == SIGUSR2){
		//Lock and then increase the counter for received SIGUSR2 signals.
        	pthread_mutex_lock(&(shm_ptr->mutex_gotUSRtwo));
        	shm_ptr->sig_gotUSRtwo++;
        	pthread_mutex_unlock(&(shm_ptr->mutex_gotUSRtwo));

//		printf("SIGUSR2 RECEIVED %d\n", shm_ptr->sig_gotUSRtwo);

		//Collect the current time value. Note that start_time is not zero.
		current_time = time(NULL);
		current_time = current_time - start_time;

		//Add the time since the last SIGUSR2 signal to the timesum variable.
		if(shm_ptr->sig_gotUSRtwo < 2){
			time_since_last_sigtwo = current_time;
			shm_ptr->timesum_sigtwo = current_time;
		} else {
			shm_ptr->timesum_sigtwo = (shm_ptr->timesum_sigtwo + (current_time - time_since_last_sigtwo));
			time_since_last_sigtwo = current_time;
		}

	//receive signal, ignore the other
	} else if (signal_number == SIGUSR1){
		//Lock and then increase the counter for received SIGUSR1 signals.
		pthread_mutex_lock(&(shm_ptr->mutex_gotUSRone));
		shm_ptr->sig_gotUSRone++;
		pthread_mutex_unlock(&(shm_ptr->mutex_gotUSRone));

//		printf("SIGUSR1 RECEIVED %d\n", shm_ptr->sig_gotUSRone);

		//Collect the current time value. Note that start_time is not zero.
		current_time = time(NULL);
		current_time = current_time - start_time;

		//Add the time since the last SIGUSR1 signal to the timesum variable.
		if(shm_ptr->sig_gotUSRone < 2){
			time_since_last_sigone = current_time;
                        shm_ptr->timesum_sigone = current_time;
		} else {
			shm_ptr->timesum_sigone = (shm_ptr->timesum_sigone + (current_time - time_since_last_sigone));
                        time_since_last_sigone = current_time;
		}
	}

}

/*This function works by running in an infinite signal generating loop. The signal generator picks a random number
 *between 1 - 10, and determines whether the number is even or odd. If it is even, then the generator will send a
 *SIGUSR2 signal to to all processes, then sleep for a random period of time. Then it will lock shared memory,
 *increment the corresponding count variable, then unlock. Following that, it calculates the amount of time since
 *the last signal OF THE SAME TYPE, and adds that to a total timesum variable. The purpose of the timesum variable
 *is so that the average time of reception of each signal type can be calculated in the report process.
 */
void signal_generator(){
	int random_num, random_sleep;
	pid_t pid;

	sleep(1);

	while(1){
		random_num = (rand() % (GEN_MAX - GEN_MIN + 1)) + GEN_MIN;
		if((random_num % 2) == 0){
			//pick sigusr2, send signal
			kill(0, SIGUSR2);
//			printf("Execute kill\n");
			//Wait a random interval of time between 0.1 and 0.01.
			random_sleep = (rand() % (DELAY_MAX - DELAY_MIN + 1)) + DELAY_MIN;
			usleep(random_sleep);

			//Lock, increment counter for SIGUSR2(sent), then unlock.
			pthread_mutex_lock(&(shm_ptr->mutex_sentUSRtwo));
			shm_ptr->sig_sentUSRtwo++;
			pthread_mutex_unlock(&(shm_ptr->mutex_sentUSRtwo));

//			printf("SIGUSR2 SENT %d\n", shm_ptr->sig_sentUSRtwo);
		} else {
			//pick sigusr1, send signal
			kill(0, SIGUSR1);
				//NOTE: first variable is PID, which is zero, meaning the signal is going to be
				//sent to ever process in the process group, meaning all 8 processes since they
				//began from the same parent.

//			printf("Execute kill\n");

			//Wait a random interval of time between 0.1 and 0.01.
			random_sleep = (rand() % (DELAY_MAX - DELAY_MIN + 1)) + DELAY_MIN;
                	usleep(random_sleep);

			//Lock, increment counter for SIGUSR1(sent), then unlock.
			pthread_mutex_lock(&(shm_ptr->mutex_sentUSRone));
			shm_ptr->sig_sentUSRone++;
			pthread_mutex_unlock(&(shm_ptr->mutex_sentUSRone));

//			printf("SIGUSR1 SENT %d\n", shm_ptr->sig_sentUSRone);
		}
	}
}

/*The report process accepts all signals, and prints out a statistics report to the screen. It also kills
 *all processes when 100,000 signals have been sent, or 30 seconds have passed.
 */
void report(int signal_number){

//	printf("Signal number in report is %d.\n", signal_number);
	//The purpose of the miniCounter is to count to ten so that the function can report.
	//int miniCounter = 0;
	time_t current_time;

	int sentUSRone = shm_ptr->sig_sentUSRone;
	int gotUSRone = shm_ptr->sig_gotUSRone;
	int sentUSRtwo = shm_ptr->sig_sentUSRtwo;
	int gotUSRtwo = shm_ptr->sig_gotUSRtwo;
	time_t timesum_sigone = shm_ptr->timesum_sigone;
	time_t timesum_sigtwo = shm_ptr->timesum_sigtwo;

	//Calculate the number of lost signals.
	int sig_two_lost = shm_ptr->sig_sentUSRtwo - shm_ptr->sig_gotUSRtwo;
	int sig_one_lost = shm_ptr->sig_sentUSRone - shm_ptr->sig_gotUSRone;

	//In the event that the variables were ordered incorrectly when subtracted, negate the sum.
	if (sig_two_lost < 0){
		sig_two_lost = sig_two_lost * -1;
	}

	if (sig_one_lost < 0){
		sig_one_lost = sig_one_lost * -1;
	}

//	while(1){
		if(shm_ptr->reportCount < 10){ //Increments the miniCounter (for when 10 signals reached)
			pthread_mutex_lock(&(shm_ptr->mutex_reportCount));
			shm_ptr->reportCount++;
			pthread_mutex_unlock(&(shm_ptr->mutex_reportCount));
//			printf("The value of miniCounter is %d.\n", shm_ptr->reportCount);
		} else { //Prints out statistics report
			current_time = time(NULL);

//			pthread_mutex_lock(&(shm_ptr->mutex_sentUSRone));
//			pthread_mutex_lock(&(shm_ptr->mutex_sentUSRtwo));
//			pthread_mutex_lock(&(shm_ptr->mutex_gotUSRone));
//			pthread_mutex_lock(&(shm_ptr->mutex_gotUSRtwo));

//			miniCounter = 0;
			puts("\n\n---10 SIGNAL REPORT---");
			printf("SYSTEM TIME(seconds): %ld\n", current_time - start_time);

			printf("\nSIGUSR1 Sent: %d\n", sentUSRone);
			printf("SIGUSR1 Received: %d\n", gotUSRone);
			printf("Lost SIGUSR1 signals: %d\n", sig_one_lost);
			printf("Avg Time between Receptions: %ld\n", gotUSRone/timesum_sigone);

			printf("\nSIGUSR2 Sent: %d\n", sentUSRtwo);
			printf("SIGUSR2 Received: %d\n", gotUSRtwo);
			printf("Lost SIGUSR2 signals: %d\n", sig_two_lost);
			printf("Avg Time between Receptions: %ld", gotUSRtwo/timesum_sigtwo);

			pthread_mutex_lock(&(shm_ptr->mutex_reportCount));
			shm_ptr->reportCount = 0;
                        pthread_mutex_unlock(&(shm_ptr->mutex_reportCount));

//			pthread_mutex_unlock(&(shm_ptr->mutex_sentUSRone));
//			pthread_mutex_unlock(&(shm_ptr->mutex_sentUSRtwo));
//			pthread_mutex_unlock(&(shm_ptr->mutex_gotUSRone));
  //                      pthread_mutex_unlock(&(shm_ptr->mutex_gotUSRtwo));
		}
//	}

	sleep(1);

	//Kills all processes if 100,000 signals are reached, or if 30 seconds have passed.
	if((shm_ptr->sig_sentUSRone + shm_ptr->sig_sentUSRone) >= 100000){
		kill(0, SIGKILL);
	} else if ((current_time - start_time) == 30){
		kill(0, SIGKILL);
	}
}
