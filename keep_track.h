#ifndef _KEEP_TRACK_H_
#define _KEEP_TRACK_H_

#include <stdlib.h>
#include <stdint.h>

/* received data packet record */
struct packet_record {
	unsigned seq;
	unsigned length;
	struct packet_record * next;
};

/* sent packets that are not "ack"ed, could be GET packet waits for DATA */
struct sent_packet {
	unsigned seq;
	unsigned timestamp;
	struct sent_packet * next;
};

/*******************************************************
 * "public" functions
 *******************************************************/

/* initialize a new download stream, return 0 on error */
int start_download(int peer_id, uint8_t * chunk_hash);

/* close a download stream, may because it is finished or denied */
int abort_download(int peer_id);

/* init the stream tracker */
int init_tracker(int max);

/* keep record of the new data packet of seq and len
 * and return the last continous seq number */
unsigned track_data_packet(int index, unsigned seq, unsigned len);

/* track ack packet (move the acked pointer, probably)
 * and count dup acks 
 * if dup ack == 3, activate fast retransmission 
 * control congestion window */
unsigned track_ack_packet(int index, unsigned seq);

/* get_chunk_hash - map peer_id to its corresponding hash */
uint8_t * get_chunk_hash(int peer_id);

/* initialize a new upload stream, return 0 on error */
int start_upload(int peer_id);

/* close a upload stream, may because it is finished or closed by peer */
int abort_upload(int peer_id);

/* return the first timeout seq, 0 if no timeout */
unsigned get_timeout_seq(int peer_id);

/* send a DATA packet, wait for ack */
int wait_ack(int peer_id, unsigned seq);

/* receive a ACK packet, clean wait ack queue */
int receive_ack(int peer_id, unsigned seq);

/* map id to index, used when controlling cwnd */
int get_upload_index_by_id(int id);

/* clean timeout connection. abort connections that have no response for 
 * a long time, guessing that peer has trouble */
void clean_timeout();

/* get the number of outstanding packets */ 
int get_queue_size(int peer_id);

/* get the last transmitted packet */
unsigned get_tail_seq_number(int peer_id);

#endif
