#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <sys/time.h>

#define DEBUG

int cond;

pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_pcond = PTHREAD_COND_INITIALIZER;

void *t1_main(void *unused);
void *t2_main(void *unused);

void *t1_main(void *unused) {
    for (int i = 0; i < 3; i++) {
	// toggle the value of cond and sleep
	cond = cond ? 0 : 1;
	sleep(1);
	if (cond) {
	    pthread_cond_signal(&cond_pcond);
	}
    }
    exit(EXIT_SUCCESS);

    // this is never reached
    return NULL;
}

void *t2_main(void *unused) {
    struct timespec timeToWait;
    struct timeval now;
    int ret;
    gettimeofday(&now, NULL);

    time_t nowTime;
    nowTime = now.tv_sec;

    timeToWait.tv_sec = nowTime;
    timeToWait.tv_nsec = (now.tv_usec+1000UL*100)*1000UL; // 100 ms

    while (1) {
	pthread_mutex_lock(&cond_mutex);
	while (!cond) {
	    ret = pthread_cond_timedwait(&cond_pcond, &cond_mutex, &timeToWait);
	    printf("t2_main wakeup ret == %d\n", ret);
	    if (ret) {
		//printf("pthread_cond_timedwait() returned error %d\t", ret);
		if (ret == ETIMEDOUT) {
		    printf("\tret == ETIMEDOUT\n");
		}
	    }
	}

#ifdef DEBUG
	printf("Timespec wait: %ld seconds %lu usec\n", timeToWait.tv_sec,
		    timeToWait.tv_nsec);
	//printf("t2_main has mutex\n");
#endif

	pthread_mutex_unlock(&cond_mutex);
    }

    return NULL;
}

void *t3_main(void *unused) {
    struct timespec timeToWait;
    struct timeval now;
    int ret;
    gettimeofday(&now, NULL);

    time_t nowTime;
    nowTime = now.tv_sec;

    timeToWait.tv_sec = nowTime;
    timeToWait.tv_nsec = (now.tv_usec+1000UL*100)*1000UL; // 100 ms

    while (1) {
	pthread_mutex_lock(&cond_mutex);
	while (!cond) {
	    ret = pthread_cond_timedwait(&cond_pcond, &cond_mutex, &timeToWait);
	    printf("t2_main wakeup ret == %d\n", ret);
	    if (ret) {
		//printf("pthread_cond_timedwait() returned error %d\t", ret);
		if (ret == ETIMEDOUT) {
		    printf("\tret == ETIMEDOUT\n");
		}
	    }
	}

#ifdef DEBUG
	printf("Timespec wait: %ld seconds %lu usec\n", timeToWait.tv_sec,
		    timeToWait.tv_nsec);
	//printf("t2_main has mutex\n");
#endif

	pthread_mutex_unlock(&cond_mutex);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t t1, t2;

    // Toggles the state of cond
    pthread_create(&t1, NULL, t1_main, NULL);

    pthread_create(&t2, NULL, t2_main, NULL);

    pthread_exit((void *) 0);
}
