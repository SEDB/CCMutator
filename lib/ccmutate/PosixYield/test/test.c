#include <pthread.h>
#include <sched.h>

int main(int argc, char *argv[]) {
    pthread_yield();
    sched_yield();

    return 0;
}
