#include <pthread.h>
#include <stdio.h>

void *thread_1(void *args) {
    pthread_t t1;

    pthread_join(t1, NULL);
    return NULL;
}


int main() {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, thread_1, NULL);
    pthread_create(&t2, NULL, thread_1, NULL);

    int ret = pthread_join(t1, NULL);
    printf("pthread_join returned %d for thread 1\n", ret);

    pthread_join(t2, NULL);

    return 0;
}
