/* It is reasonable to view connections with different peers separately 
 * and thus track different connection separately */

#include "keep_track.h"
#include "sha.h"
#include "util.h"
#include <time.h>

#define ABS(x) ((x)>=(0)?(x)(-x))
/* no valid identity can be 0 */
#define ID_NULL	0
/* fixed chunk size of 512 KB */
#define CHUNK_SIZE	512 * 1024

/* static variables, only used in this file */
/* of max downloads size, each can be a separate transmission stream */
static struct packet_record ** last_acked_record;
static struct sent_packet ** sent_queue_head, ** sent_queue_tail;
static int * download_id_map;
static char ** download_chunk_map; 
static int * upload_id_map;
static int max_conns;

/* network parameters */
static unsigned RTT = 0;
static unsigned deviation = 0;
static unsigned RTO;

/* "private" function prototypes */

/* estimate the RTT and network deviation */
void infer_RTT(unsigned timestamp); 
int finish_download(int index);
int get_download_index_by_id(int id);
int add_record(struct packet_record * root, unsigned seq, unsigned len);


/* init the stream tracker */
int init_tracker(int max) {
	max_conns = max;
	last_acked_record = malloc(max * sizeof(struct packet_record *));
	sent_queue_head = malloc(max * sizeof(struct sent_packet *));
	download_id_map = malloc(max * sizeof(int));
	upload_id_map = malloc(max * sizeof(int));
	download_chunk_map = malloc(max * sizeof(char *));	
	if(download_chunk_map == NULL)
		return 0;
	memset(sent_queue_head, 0, max * sizeof(struct sent_packet *));
	memset(last_acked_record, 0, max * sizeof(struct packet_record *));
	memset(download_id_map, 0, max * sizeof(int));
	memset(up_id_map, 0, max * sizeof(int));
	return 1;
}

/* Receive data: keep record of the new data packet of seq and len
 * and return the last continous seq number 
 * inform user if transmission is completed */
unsigned track_data_packet(int peer_id, unsigned seq, unsigned len) {
	int index = get_download_index_by_id(peer_id);
	int last_continous_seq;	
	struct packet_record * root, * node, * tmp;
	
	/* tracker will stop after last ACK is sent; it is possible that 
	 * the last ACK is lost, so if receive a data packet not found in
	 * the tracker, politely reply an ACK, although it is useless */
	if(index == -1) {
		return seq;
	}
	
	root = last_acked_record[index];
	
	/* add new record to the tracker */
	add_record(root, seq, len);
	/* find the largest continous seq */
	node = root;
	while(node -> next != NULL && 
		node -> seq + node -> length == node -> next -> seq) {
		node = node -> next;
	}
		
	/* delete acked node to speed up */
	while(root != node) {
		tmp = root;
		root = root -> next;
		free(tmp);
	}
	last_continous_seq = root -> seq;
	last_acked_record[index] = root;
	
	if(last_continous_seq + root -> length == CHUNK_SIZE) {
		/* downloading completed */	
		finish_download(index);
		free(root);
	}
	
	return last_continous_seq;
}

/* map peer id to its corresponding chunk hash */
char * get_chunk_hash(int peer_id) {
	int index = get_download_index_by_id(peer_id);
	return index == -1 ? NULL : download_chunk_map[index];
}

int start_download(int peer_id, char * chunk_hash) {
	int i;
	for(i = 0; i < max_conns; i++) {
		if(download_id_map[i] == ID_NULL) {
			download_id_map[i] = peer_id;
			download_chunk_map[i] = malloc(SHA1_HASH_SIZE);
			memcpy(download_chunk_map[i], chunk_hash, SHA1_HASH_SIZE);
			/* seq will start at 1, so add a root node to indicate this fact */
			track_data_packet(peer_id, 0, 1);
			return 1;
		}
	}
	return 0;
}

/* close a download stream, may because it is finished or denied */
int abort_download(int peer_id) {
	int index = get_download_index_by_id(peer_id);
	download_id_map[index] = ID_NULL;
	/* release resource */
	free(download_chunk_map[index]);
	return 1;
}

int start_upload(int peer_id) {
	int i;
	for(i = 0; i < max_conns; i++) {
		if(upload_id_map[i] == ID_NULL) {
			upload_id_map[i] = peer_id;
			return 1;
		}
	}
	return 0;
}

/* close a upload stream, may because it is finished or closed by peer */
int abort_upload(int peer_id) {
	int index = get_upload_index_by_id(peer_id);
	upload_id_map[index] = ID_NULL;
	/* TODO: release resource */
	return 1;
}

/* return the first timeout seq, 0 if no timeout */
unsigned get_timeout_seq(int peer_id) {
	int index = get_upload_index_by_id(peer_id);
	struct sent_packet * head = sent_queue_head[index];
	unsigned = seq;
	if(milli_time() - head -> timestamp > RTO) {
		sent_queue_head[index] = head -> next;
		seq = head -> seq;.
		free(head);
		return seq;
	}
	else
		return 0;
}

/* send a DATA packet, wait for ack; enqueue */
int wait_ack(int peer_id, unsigned seq) {
	int index = get_upload_index_by_id(peer_id);
	struct sent_packet * tail = sent_queue_tail[index];
	struct sent_packet * new_node = malloc(sizeof(struct sent_packet));
	new_node -> seq = seq;
	new_node -> timestamp = milli_time(); /* get current timestamp */
	new_node -> next = NULL;
	if(tail == NULL) {
		sent_queue_tail[index] = new_node;
		sent_queue_head[index] = new_node;
	}
	else {
		tail -> next = new_node;
		tail = new_node;
	}
}

/* receive a ACK packet, clean wait ack queue 
 * return the number of packets acked */
int receive_ack(int peer_id, unsigned seq) {
	int count = 0;
	int index = get_upload_index_by_id(peer_id);
	struct sent_packet * head = sent_queue_head[index];
	struct sent_packet * tmp;
	/* cumulative ack, clear all in front of queue
	 * whose seq equal to or less than seq; retransmission may cause 
	 * reordering, but node is not the head doesn't affect timeout */
	/* TODO: may consider double-ended queue to solve reordering problem */
	while(head != NULL && head -> seq <= seq) {
		if(head -> seq == seq)
			infer_RTT(head -> timestamp);
		tmp = head;
		head = head -> next;
		free(tmp);
		count++;
	}
	/* TODO: three dup acks should invoke fast retransmission */
	sent_queue_head[index] = head;
	return count;
}


/*******************************************************
 * helper functions
 *******************************************************/

/* estimate the RTT and network deviation */
void infer_RTT(unsigned timestamp) {
	unsigned new_RTT = timestamp - milli_time();
	unsigned new_dev;
	RTT = ALPHA * RTT + (1 - ALPHA) * new_RTT;
	new_dev = ABS(new_RTT - RTT);
	RTO = RTT + 2 * new_dev;/* inaccurate, TODO */
}

int finish_download(int index) {
	int peer_id = download_id_map[index];
	/* TODO: inform user */
	
	abort_download(peer_id);
}

/* map peer id to download stream index, -1 on error */
int get_download_index_by_id(int id) {
	int i;
	for(i = 0; i < max_conns; i++) {
		if(download_id_map[i] == id)
			return i;
	}
	return -1;
}

/* map peer id to upload stream index, -1 on error */
int get_upload_index_by_id(int id) {
	int i;
	for(i = 0; i < max_conns; i++) {
		if(upload_id_map[i] == id)
			return i;
	}
	return -1;
}

int add_record(struct packet_record * root, unsigned seq, unsigned len) {
	struct packet_record * new_node;
	
	if(root == NULL) {
		root = malloc(sizeof(struct packet_record));
		root -> next = NULL;
		root -> seq = seq;
		root -> length = len;
		return 1;
	}
	/* 1 -> 1000 (root) -> seq -> 3000*/
	while(root -> next != NULL && root -> next -> seq < seq)
		root = root -> next;
	new_node = malloc(sizeof(struct packet_record));
	new_node -> seq = seq;
	new_node -> length = len;
	new_node -> next = root -> next;
	root -> next =  new_node;
	return 1;
}
