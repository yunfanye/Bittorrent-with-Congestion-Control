#ifndef _COMMON_H_
#define _COMMON_H_

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "sha.h"

#define FILE_NAME_SIZE	255

#define OWNED 0
#define RECEIVING 1
#define NOT_STARTED 2

#define WORKING 0
#define NOT_WORKING 1

#define MAX_LINE_LENGTH 255
#define CHUNK_HASH_SIZE 41
#define MAGIC_NUMBER 15441
#define VERSION_NUMBER 1
#define HEADER_LENGTH 16
#define MAX_PACKET_SIZE 1500
#define MAX_PAYLOAD_SIZE 1484
#define MAX_PACKET_PER_CHUNK 354

#define CHUNK_NUMBER_AND_PADDING_SIZE 4

#define WHOHAS 0
#define IHAVE 1
#define GET 2
#define DATA 3
#define ACK 4
#define DENIED 5

#define CHUNK_PER_PACKET 74

/* each chunk */
struct Chunk {
	int id;
	uint8_t hash[SHA1_HASH_SIZE];
	int state;	/* 0 owned, 1 receiving */
	char* data;
	int received_seq_number;
	int received_byte_number;
};

struct Request{
	char* filename;
    int chunk_number;
    struct Chunk* chunks;
};

struct packet{
    char* header;
    char payload[MAX_PAYLOAD_SIZE];
};

struct connection{
	// used fields
	int peer_id;

	int chunk_count;
	struct Chunk* chunks;
	struct connection* next;
	int state;
	// unused fields
	struct sockaddr_in addr;

	int ssthresh;
	int window_size;
	
	int last_ack_number;
	int last_sent_number;
	int number_duplicate_ack;

	int RTT;

	struct timeval CONGESTION_AVOIDANCE_timestamp;
	struct timeval last_ack_timestamp;
	struct timeval last_data_timestamp;
	struct timeval last_get_timestamp;
};



/* owned sha1 hash */
struct Chunk *chunkTable;

#endif
