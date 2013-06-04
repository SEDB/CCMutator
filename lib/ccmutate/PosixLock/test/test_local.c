#include <pthread.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    pthread_mutex_t mut1, mut2;
    pthread_mutex_init(&mut1, NULL);
    pthread_mutex_init(&mut2, NULL);

    pthread_mutex_lock(&mut1);
    int a = argc;
    if (rand() % 2) {
	pthread_mutex_unlock(&mut1);
	return 0;
    }
    pthread_mutex_lock(&mut2);
    a = a + 1;
    pthread_mutex_unlock(&mut2);
    pthread_mutex_unlock(&mut1);

    return 0;
}

