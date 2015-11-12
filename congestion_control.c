#include "keep_track.h"
#include "congestion_control.h"
#include <stdio.h>
#include "window_log.h"
#include "util.h"

#define INITIAL_WINDOW	1

#define MAX(x, y)	((x)>(y)?(x):(y))

int init_controller(int max) {
	cwnds = malloc(max * sizeof(unsigned));
	max_cwnds = malloc(max * sizeof(unsigned));
	ack_counts = malloc(max * sizeof(unsigned));
	cc_policies = malloc(max * sizeof(enum cc_policy));
	return 1;
}

int window_control(int peer_id, int count) {
	int index = get_upload_index_by_id(peer_id);
	int window_size_change = 0;
	if(index == -1)
		return -1;
	if(cc_policies[index] == slow_start) {
		cwnds[index] += count;
		max_cwnds[index] = MAX(cwnds[index], max_cwnds[index]);
		window_size_change = 1;
	}
	else if (cc_policies[index] == congestion_avoidance) {
		if(cwnds[index] <= max_cwnds[index]/2) {
			/* current wnd reachs half of max, linear increase */
			ack_counts[index] += count;
			if(ack_counts[index] >= cwnds[index]) {
				ack_counts[index] -= cwnds[index];
				cwnds[index]++;	
				window_size_change = 1;		
			}
		}
		else {
			cwnds[index] += count;
			window_size_change = 1;
		}
	}
	if(window_size_change)
		log_window(peer_id, cwnds[index], milli_time());
	return 1;
}

void window_timeout(int peer_id) {
	int index = get_upload_index_by_id(peer_id);
	if(index == -1)
		return;
	cc_policies[index] = congestion_avoidance;
	cwnds[index] = INITIAL_WINDOW;
	log_window(peer_id, cwnds[index], milli_time());
}

int get_cwnd_size(int peer_id) {
	int index = get_upload_index_by_id(peer_id);
	if(index != -1){
		return cwnds[index];
	}
	return -1;
}

void init_cwnd(int peer_id) {
	int index = get_upload_index_by_id(peer_id);
	if(index == -1)
		return;
	cc_policies[index] = slow_start;
	cwnds[index] = INITIAL_WINDOW;
	max_cwnds[index] = 1;
}
