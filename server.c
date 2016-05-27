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
#include <wiringPi.h>

#define error_handler(eN, msg)\
    do {errno = eN; perror(msg); exit(EXIT_FAILURE); } while(0)

#define PORT 3344
#define MAXCONNS 1

int ssock;
struct pollfd poll_list[MAXCONNS + 1];

void handle_new_connection() {
  // TODO: bejövő kapcsolat kezelése
  // Ha van üres hely a tömbben, akkor oda tesszük egyébként visszautasítjuk.
  int csock, i;

  // csock = accept(ssock, NULL, NULL);
  if ((csock = accept(ssock, NULL, NULL)) < 0) {
      return;
  }
  printf("INCOMING CONNECTION\n");
  for (i = 1; i < MAXCONNS+1; i++) {
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

void process_read(int csock) {
  char buf[256];
  int len;
  int i,lvl,mode;

  len = recv(csock, buf, sizeof(buf), 0);
//  printf("RECEIVED %d CHAR\n", len);
  if (len > 0) {
//	 TODO service routine
	//	for (i = 0; i < sizeof(buf); ++i) {
	//		printf("0x%02X ",buf[i]);
	//	}
	//	printf("\n\n");
//	printf("%d %d %d\n", buf[0],buf[1],buf[2]);
	switch (buf[0]) {
		case 0x41: // A: set pin mode
			printf("setting pin: %d to mode: %s\n", (buf[1]-0x30), ( (buf[2]-0x30)==0) ? "OUTPUT" : ((buf[2]-0x30)==1) ? "INPUT" : "ERROR" );
//			TODO hibakezeles es sanitizing
			pinMode((buf[1]-0x30), ((buf[2]-0x30)==0) ? OUTPUT : ((buf[2]-0x30)==1) ? INPUT : INPUT);
			break;

		case 0x42: // B: set pin level
			printf("writing to pin: %d level: %s\n", (buf[1]-0x30), ( (buf[2]-0x30)==0) ? "LOW" : ((buf[2]-0x30)==1) ? "HIGH" : "ERROR" );
//			TODO hibakezeles es sanitizing
			digitalWrite((buf[1]-0x30), ((buf[2]-0x30)==0) ? LOW : ((buf[2]-0x30)==1) ? HIGH : LOW);
			break;

		case 0x43: // C: read pin level
//			TODO hibakezeles es sanitizing
			lvl = digitalRead((buf[1]-0x30));
			printf("reading pin: %d level: %s\n", (buf[1]-0x30), (lvl ? "HIGH" : "LOW") );
			if(send(csock, &lvl, 1 , 0) < 0){ // a single byte is sufficient
			  perror("send");
			}
			break;

		case 0x44: //D: read pin mode
//			TODO hibakezeles es sanitizing
			mode = getAlt((buf[1]-0x30));
			printf("reading pin: %d mode: %s\n", (buf[1]-0x30), (mode ? "OUTPUT" : "INPUT") );
			if(send(csock, &mode, 1 , 0) < 0){ // a single byte is sufficient
			  perror("send");
			}
			break;

		case 0x59: //Y: read all
			printf("reading all pins\n");
			for (i = 0; i <= 31; ++i) {
				mode = getAlt(i);
				printf("GPIO: %d MODE: %s ",i,(mode ? "OUTPUT" : "INPUT"));
				if(send(csock, &mode, 1, 0) < 0){
					perror("send");
				}
				lvl = digitalRead(i);
				printf("LVL: %s\n",(lvl ? "HIGH" : "LOW"));
				if(send(csock, &lvl, 1, 0) < 0){
					perror("send");
				}
			}
			break;
		case 0x58: //X: close conn
			printf("closing conn no:%d\n",csock);
			close(csock);
			poll_list[1].fd = -1;
			break;

		default:
			break;
	}


//      for (i = 1; i <= MAXCONNS ; i++) {
//          // send only for connected clients, do not send message back to sender
//          if ((poll_list[i].fd != -1) && (poll_list[i].fd != csock)) {
//              if(send(poll_list[i].fd, buf, len , 0) < 0){
//                  perror("send");
//              }
//
//          }
//      }
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

    printf("initializing wiringPi...\n");
	wiringPiSetup();

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
	    if(poll(poll_list, MAXCONNS + 1, -1) > 0){
	    	if(poll_list[0].revents & POLLIN){
	    		handle_new_connection();
	    	}

	    	for(i = 1; i <= MAXCONNS; i++){
	    		if(poll_list[i].revents & (POLLERR | POLLHUP)){
	    			close(poll_list[i].fd);
	    			poll_list[i].fd = -1;
	    		}
	    		else if(poll_list[i].revents & POLLIN){
	    			process_read(poll_list[i].fd);
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
