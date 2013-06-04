#include <pthread.h>

int main() {
    pthread_cond_t *cond;
    pthread_cond_init(cond, NULL);

    pthread_cond_signal(cond);

    pthread_cond_broadcast(cond);

    return 0;
}
