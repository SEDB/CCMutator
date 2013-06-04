/*
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
*/

#include <condition_variable>
#include <mutex>
#include <thread>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ratio>
#include <chrono>

#define DEBUG

bool cond;

std::mutex cond_mutex;
std::condition_variable cond_pcond;

void t1_main(void);
void t2_main(void);

void t1_main(void) {
    for (int i = 0; i < 3; i++) {
	// toggle the value of cond and sleep
	cond = cond ? false : true;
	if (cond) {
            printf("waking up a thread\n");
            cond_pcond.notify_one();
	}
	sleep(1);
    }
    printf("Exiting\n");
    exit(EXIT_SUCCESS);
}

void t2_main(void) {
    while (1) {
        printf("t2 running\n");
        std::unique_lock<std::mutex> lk(cond_mutex);
        std::chrono::nanoseconds relTime(5672);
        bool ret = cond_pcond.wait_for(lk, relTime, []{ return cond;});
        if (!ret) {
            printf("t2 timeout\n");
            continue;
        }
#ifdef DEBUG
	printf("t2_main has mutex\n");
#endif
    } // implicit unlock 
}

void t3_main(void) {
    while (1) {
        printf("t3 running\n");
        std::unique_lock<std::mutex> lk(cond_mutex);
	while (!cond) {
            std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
            std::cv_status ret = cond_pcond.wait_until(lk, now + std::chrono::nanoseconds(5672));
            if (ret == std::cv_status::timeout) {
                printf("t3 timed out\n");
                continue;
            }
	    printf("t3_main wakeup\n");
	}
#ifdef DEBUG
	printf("t3_main has mutex\n");
#endif
    }
}

int main(int argc, char *argv[]) {
    /*
    pthread_t t1, t2, t3;

    // Toggles the state of cond
    pthread_create(&t1, NULL, t1_main, NULL);

    pthread_create(&t2, NULL, t2_main, NULL);
    pthread_create(&t3, NULL, t3_main, NULL);

    pthread_exit((void *) 0);
    */

    printf("Program started\n");

    std::thread t1(t1_main);
    std::thread t2(t2_main);
    std::thread t3(t3_main);

    t1.join();
    t2.join();
    t3.join();

    return 0;
    
}
