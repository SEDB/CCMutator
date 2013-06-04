#include <pthread.h>
#include <sched.h>

int someFunc(void) {
    if (1) 
	sched_yield();

    return 0;
}

int someOtherFunc(void) {
    sched_yield();

    return 0;
}

int someDistantFunc(void) {
    sched_yield();

    return 0;
}

int main(int argc, char *argv[]) {
    pthread_yield();
    sched_yield();

    return 0;
}
