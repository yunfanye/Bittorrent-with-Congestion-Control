#include "connection.h"

void connection_scale_up(struct connection* connection){
	if(connection->state == SLOW_START){
		connection->window_size = 1 + connection->window_size;
	}
	else{
		// CONGESTION_AVOIDANCE
		struct timeval now;
		gettimeofday(&now, NULL);
		int t1 = connection->CONGESTION_AVOIDANCE_timestamp.tv_usec;
		int t2 = now.tv_usec;
		if(t2-t1>connection->RTT){
			connection->window_size = 1 + connection->window_size;
			gettimeofday(&connection->CONGESTION_AVOIDANCE_timestamp, NULL);
		}
	}
	// update connection state
	if(connection->window_size>=connection->ssthresh){
		connection->state = CONGESTION_AVOIDANCE;
	}
	else{
		connection->state = SLOW_START;
	}
}

void connection_scale_down(struct connection* connection){
	if(connection->window_size/2 > 2){
		connection->ssthresh = connection->window_size / 2;
	}
	else{
		connection->ssthresh = 2;	
	}
	if(connection->state == CONGESTION_AVOIDANCE){
		connection->window_size = INITIAL_WINDOW_SIZE;
	}
	// update connection state
	if(connection->window_size>=connection->ssthresh){
		connection->state = CONGESTION_AVOIDANCE;
	}
	else{
		connection->state = SLOW_START;
	}
}

void connection_timeout_sender(struct connection* connection){
	struct timeval now;
	gettimeofday(&now, NULL);
	int t1 = connection->last_ack_timestamp.tv_usec;
	int t2 = now.tv_usec;
	if(t2-t1>CRASH_TIMEOUT_THRESHOLD){

		return;
	}
	if(t2-t1>ACK_TIMEOUT_THRESHOLD){
		t1 = connection->last_data_timestamp.tv_usec;
		connection_scale_down(connection);	
	}
}

void connection_timeout_sender(struct connection* connection){
	struct timeval now;
	gettimeofday(&now, NULL);
	int t1 = connection->last_data_timestamp.tv_usec;
	int t2 = now.tv_usec;
	if(t2-t1>CRASH_TIMEOUT_THRESHOLD){
		cancel_receiver_connection(receiver_connection);
	 	remove_receiver_connection(receiver_connection);
		return;
	}
	t1 = connection->last_get_timestamp.tv_usec;
	if(t2-t1>DATA_TIMEOUT_THRESHOLD){
		// TODO: send a new get packet
		return;
	}
}

// if a peer crash is detected, cancel the connection with the peer
void cancel_connection(struct connection* connection){
	
}


void remove_connection(struct connection* connection, struct connection* head){
	struct connection* current = head;
	if(current!=NULL){
		if(current==connection){
			head = head->next;
			return;
		}
		while(current->next){
			if(current->next==connection){
				current->next==connection->next;
				return;
			}
			current=current->next;
		}
	}
}
