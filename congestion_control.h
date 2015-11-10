#ifndef _CONGESTION_CONTROL_H
#define _CONGESTION_CONTROL_H

enum cc_policy {
	slow_start = 1,
	congestion_avoidance = 2
};

/* cc policy for each connection */
enum cc_policy * cc_policies;

/* current cwnd size for each connection */
unsigned * cwnds;

/* max cwnd size for each connection */
unsigned * max_cwnds;

/* ack counts used in congestion avoidance phase */
unsigned * ack_counts;

int init_controller(int max);

/* return cwnd size of the specifid peer */
int get_cwnd_size(int peer_id);

/* control cwnd window */
int window_control(int peer_id, int count);

/* call window_timeout when a packet timeouts */
void window_timeout(int peer_id);

void init_cwnd(int peer_id);

#endif
