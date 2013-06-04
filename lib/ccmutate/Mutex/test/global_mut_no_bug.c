#include <pthread.h>
#include <stdio.h>

pthread_mutex_t mut1;
pthread_mutex_t mut2;

void *t1_main(void *unused);
void *t2_main(void *unused);

void *t2_main(void *unused) {
    pthread_mutex_lock(&mut1);
    printf("t2 has mut2\n");
    pthread_mutex_lock(&mut2);
    printf("t2 has mut2\n");
    pthread_mutex_unlock(&mut2);
    printf("t2 released mut2\n");
    pthread_mutex_unlock(&mut1);
    printf("t2 released mut2\n");

    return NULL;
}

void *t1_main(void *unused) {
    pthread_mutex_lock(&mut1);
    printf("t1 has mut1\n");
    pthread_mutex_lock(&mut2);
    printf("t1 has mut2\n");
    pthread_mutex_unlock(&mut2);
    printf("t1 released mut2\n");
    pthread_mutex_unlock(&mut1);
    printf("t1 released mut1\n");

    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t t1;
    pthread_t t2;

    pthread_mutex_init(&mut1, NULL);
    pthread_mutex_init(&mut2, NULL);

    pthread_create(&t1, NULL, t1_main, NULL);
    pthread_create(&t2, NULL, t2_main, NULL);

    pthread_exit((void *) 0);
}
