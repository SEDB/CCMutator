#include <thread>
#include <mutex>
#include <stdlib.h>
#include <pthread.h>
#include <cstdio>

// main() has 6 lock-unlock pairs and other_func also has 6

int other_func();

int main(int argc, char *argv[]) {
    printf("Test starting\n");
    std::mutex mut1, mut2;
    pthread_mutex_t mut3, mut4;
    pthread_mutex_init(&mut3, NULL);
    pthread_mutex_init(&mut4, NULL);

    mut1.lock();                        // pair 1 (mut 1)
    pthread_mutex_lock(&mut4);          // pair 5 (mut 4)
    int a = argc;
    if (a % 2) {
        mut1.unlock();                  // pair 1 (mut 1)
        pthread_mutex_unlock(&mut4);    // pair 6 (mut 4)
        printf("Early abort\n");
	return 1;
    }
//    other_func();
    mut2.lock();                        // pair 2 (mut 2)
    pthread_mutex_lock(&mut3);          // pair 4 (mut 3)
    a = a + 1;
    pthread_mutex_unlock(&mut3);        // pair 4 (mut 3)
    mut2.unlock();                      // pair 2 (mut 2)
    mut1.unlock();                      // pair 3 (mut 1)
    pthread_mutex_unlock(&mut4);        // pair 5 (mut 4)

    printf("a == %d\n", a);

    return 0;
}

#if 0
int other_func() {
    printf("Other func starting\n");
    std::mutex mut1, mut2;
    pthread_mutex_t mut3, mut4;
    pthread_mutex_init(&mut3, NULL);
    pthread_mutex_init(&mut4, NULL);

    mut1.lock();
    pthread_mutex_lock(&mut4);
    int b = rand() % 10 + 1;
    if (b % 2) {
        mut1.unlock();
        pthread_mutex_unlock(&mut4);
        printf("Early other func abort\n");
	return 1;
    }
    mut2.lock();
    pthread_mutex_lock(&mut3);
    b = b + 1;
    pthread_mutex_unlock(&mut3);
    mut2.unlock();
    mut1.unlock();
    pthread_mutex_unlock(&mut4);

    printf("b == %d\n", b);
    return 0;
}
#endif
