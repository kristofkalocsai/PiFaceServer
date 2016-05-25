/*
 * server.c
 *
 *  Created on: May 24, 2016
 *      Author: kristofkalocsai
 */

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

#define error_handler(eN, msg)\
    do {errno = eN; perror(msg); exit(EXIT_FAILURE); } while(0)


void* thread_fun(void* arg) {
    int i,j,count,policy;
    double k,m,elapsedtime,avgtime,sumtime=0;
    struct timeval t1,t2;
    struct sched_param param;

    pthread_getschedparam(pthread_self(), &policy , &param);

    printf("starting aux thread with...\n");
    printf("policy: %s, prio: %d\n", policy == SCHED_FIFO ? "FIFO" :\
                                    (policy == SCHED_RR ? "RR" :\
                                    (policy == SCHED_OTHER ? "default" :\
                                    "unknown" )) \
                                    , param.sched_priority);


    count = 100000;

    printf("measuring avg time for %d cycles...\n", count);

    for (j = 0; j < count; j++) {
        k = 5.2L;
        m = 0.001L;
        gettimeofday(&t1, NULL);
        for (i = 0; i < 140000; i++) {
            k = k-k*m;
        }
        gettimeofday(&t2, NULL);


        elapsedtime = (t2.tv_sec - t1.tv_sec) * 1e3L;
        elapsedtime += (t2.tv_usec - t1.tv_usec)/ 1e3L;

        sumtime = sumtime+elapsedtime;
    }


    avgtime = sumtime/(double)count;

    printf("k:%lf, avg_d_t:%lf[ms]\n", k, avgtime);
    printf("closing aux thread...\n");
    return NULL;
}

int main(void){

    int eN;

    pthread_t mythread;
    pthread_attr_t attr;
    struct sched_param param;

    // initialize thread attributes struct
    eN = pthread_attr_init(&attr);
    if (eN != 0){
        error_handler(eN,"pthread_attr_init");
    }


    param.sched_priority = 30;

    // set scheduling policy in attributes struct
    eN = pthread_attr_setschedpolicy(&attr, SCHED_RR);
    if (eN != 0){
        error_handler(eN,"pthread_attr_setschedpolicy");
    }

    // set inheritance in attributes struct
    eN = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (eN != 0){
        error_handler(eN,"pthread_attr_setinheritsched");
    }

    // set scheduling parameters (priority) in attributes struct
    eN = pthread_attr_setschedparam(&attr, &param);
    if (eN != 0){
        error_handler(eN,"pthread_attr_setschedparam");
    }

    // create thread with previously set attributes
    eN = pthread_create(&mythread, &attr, thread_fun, NULL);
    if (eN != 0){
        error_handler(eN,"pthread_create");
    }


    // now thread does it's thing, waiting to join
    printf("waiting for aux thread to finish...\n");

    pthread_join(mythread, NULL);

    printf("main thread closes...\n");
    exit(EXIT_SUCCESS);

    return 0;
}
