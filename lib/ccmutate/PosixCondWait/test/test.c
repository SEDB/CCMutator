#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

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
    while (1) {
	pthread_mutex_lock(&cond_mutex);
	while (!cond) {
	    pthread_cond_wait(&cond_pcond, &cond_mutex);
	    printf("t2_main wakeup\n");
	}
#ifdef DEBUG
	printf("t2_main has mutex\n");
#endif

	pthread_mutex_unlock(&cond_mutex);
    }

    return NULL;
}

void *t3_main(void *unused) {
    while (1) {
	pthread_mutex_lock(&cond_mutex);
	while (!cond) {
	    pthread_cond_wait(&cond_pcond, &cond_mutex);
	    printf("t3_main wakeup\n");
	}
#ifdef DEBUG
	printf("t3_main has mutex\n");
#endif

	pthread_mutex_unlock(&cond_mutex);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t t1, t2, t3;

    // Toggles the state of cond
    pthread_create(&t1, NULL, t1_main, NULL);

    pthread_create(&t2, NULL, t2_main, NULL);
    pthread_create(&t3, NULL, t3_main, NULL);

    pthread_exit((void *) 0);
}
