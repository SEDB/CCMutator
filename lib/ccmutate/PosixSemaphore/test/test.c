#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>

sem_t *sem1;
sem_t *sem2;

int main() {

    sem1 = sem_open("sem1", O_EXCL);
    sem2 = sem_open("sem2", O_CREAT | O_EXCL, 0666, 17);
    if (sem2 == SEM_FAILED) {
	perror("sem_open() returned");
    }
    int ret = sem_init(sem2, 0, 1);

    return 0;
}
