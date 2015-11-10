#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <stdio.h>
#include <stdlib.h> 

#define SLOW_START 0
#define CONGESTION_AVOIDANCE 1

#define INITIAL_WINDOW_SIZE 1
#define INITIAL_SSHTHRESH 64

#define DUPLICATE_ACK_THRESHOLD 3

#define INITIAL_RTT 2000000 // in microseconds
#define CRASH_TIMEOUT_THRESHOLD 5000000 // in microseconds
#define ACK_TIMEOUT_THRESHOLD 500000 // in microseconds
#define DATA_TIMEOUT_THRESHOLD 4000000 // in microseconds


void connection_scale_up(struct connection* connection);
void connection_scale_down(struct connection* connection);

void connection_timeout_sender(struct connection* connection);

#endif
