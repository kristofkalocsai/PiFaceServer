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
//#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

#define error_handler(eN, msg)\
    do {errno = eN; perror(msg); exit(EXIT_FAILURE); } while(0)

#define PORT 3344
#define MAXCONNS 1

int ssock;
struct pollfd poll_list[MAXCONNS + 1];

void handle_new_connection()
{
  // TODO: bejövő kapcsolat kezelése
  // Ha van üres hely a tömbben, akkor oda tesszük egyébként visszautasítjuk.
  int csock, i;

  // csock = accept(ssock, NULL, NULL);
  if ((csock = accept(ssock, NULL, NULL)) < 0) {
      return;
  }
  printf("INCOMING CONNECTION\n");
  for (i = 0; i < MAXCONNS+1; i++) {
      if (poll_list[i].fd < 0) {
          // van ures hely
          printf("ASSIGNING %d. PLACE TO CLIENT %d\n", i, csock);
          poll_list[i].fd = csock;
          return;
      }
  }

  // nincs ures hely
  printf("SRVR FULL\n");
  send(csock, "SRVR FULL!\n", strlen("SRVR FULL!\n"), 0);
  close(csock);

}

void process_read(int csock)
{
  // TODO: beolvassuk a szöveget és szétküldjük a többieknek
  char buf[256];
  int len;
  int i;

  len = recv(csock, buf, sizeof(buf), 0);
//  printf("RECEIVED %d CHAR\n", len);
  if (len > 0) {
//	  ide kell irni a kiszolgalast
      for (i = 1; i <= MAXCONNS ; i++) {
          // send only for connected clients, do not send message back to sender
          if ((poll_list[i].fd != -1) && (poll_list[i].fd != csock)) {
              if(send(poll_list[i].fd, buf, len , 0) < 0){
                  perror("send");
              }

          }
      }
  }
}

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

    struct sockaddr_in6 addr;
    int reuse, i, ii;

    printf("opening socket\n");
    eN = (ssock = socket(PF_INET6, SOCK_STREAM, 0));
    if(eN < 0){
       error_handler(eN, "socket");
    }

    reuse = 1;

    setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_addr = in6addr_any;
	addr.sin6_port = htons(PORT);

	printf("binding socket\n");
    eN = bind(ssock, (struct sockaddr*)&addr, sizeof(addr));
    if (eN < 0){
    	error_handler(eN,"bind");
    }

    printf("listening...\n");
    eN = listen(ssock,5);
    if (eN < 0){
    	error_handler(eN,"listen");
    }

    poll_list[0].fd = ssock;
	poll_list[0].events = POLLIN;

	for(i = 1; i <= MAXCONNS; i++){
		poll_list[i].fd = -1;
		poll_list[i].events = POLLIN;
	}



	while(1){
	    // TODO: poll
	    if (poll(poll_list, MAXCONNS+1, -1) > 0) {
	        // TODO: az események feldolgozása és a lekezelő függvények meghívása
	        // jott esemeny
	        if (poll_list[0].revents & POLLIN) {
	        	printf("NEW CONNECTION\n");
	            handle_new_connection();
	        }
	        for (ii = 0; ii < MAXCONNS+1; ii++) {
	            if (poll_list[ii].revents & (POLLERR | POLLHUP)) {
	            	close(poll_list[ii].fd);
	                poll_list[ii].fd = -1;
	            }
	            else if(poll_list[ii].revents & POLLIN) {
	            	process_read(poll_list[ii].fd);
	            }
	        }

	    }
	}


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
