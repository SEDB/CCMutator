Java Mutation Operators and Potential C++ Equivalents

1. Remove call sleep()
1. Modify parameter sleep()
    Same as in C++
1. Modify parameter wait()
    wait() with a parameter becomes a timed wait(). So this would be akin to either:
	1. Modifying parameter pthread_cond_timedwait()
	2. Replacing pthread_cond_wait() with pthread_cond_timedwait()
1. Modify parameter join()
    similar to wait(), javas join accepts a parameter to unblock after a
    certain amount of time. This is not found in POSIX standard, however there are some GNU extensions:

    int pthread_tryjoin_np(pthread_t thread, void **retval);
    int pthread_timedjoin_np(pthread_t thread, void **retval,
                                const struct timespec *abstime);
    Obviously, non-portable. This could be implemented by replacing a call to
    pthread_join() with pthread_timedjoin_np().
1. Modify parameter await()
    Await waits until all threads have come to wait on a variable (sort of like a backwards semaphore).
    I cannot find a comparable feature in C++ or POSIX
1. Modify semaphore fairness
    In Java, fairness can be set true or false.
	True: FIFO order of acquires
	False: No guarantees
    I could not find how POSIX handles fairness (i think it is implementation dependents)
1. Modify barrier runnable parameter
    I believe this refers to CyclicBarrier. Similar to await() all threads wait
    for each other at the barrier. The runnable parameter is run once all
    threads have blocked on the barrier but before any of them have unblocked.
    I cannot find anything similar in C++ or POSIX
1. Remove call yield()	    (IMPLEMENTED)
    Thread yields to schedualer. 
    Non-standard: pthread_yield()
    POSIX Standard: sched_yield()
1. Remove call wait()
1. Exchange lock/permit acquisition (deadlock introduction) (IMPLEMENTED)
    Swap calls to pthread_mutex_lock
1. Remove synchronized block (IMPLEMENTED)
    Remove pair of pthread_mutex_lock and unlock
1. Replace one concurrency method with another (lock -> semaphore)
1. Shift critical region
1. Shrink critical region
1. Expand critical region
1. Split critical region
    Self explanatory, all shift a pair of pthread_mutex_lock/unlock calls

Other Possible Operators:
Modify mutex attributes
    Set recursive mutex non-recursive
    Set non-recursive mutex recursive
Modify thread attributes
