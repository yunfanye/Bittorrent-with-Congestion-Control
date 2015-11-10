#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <errno.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sha.h"
#include "common.h"
#include "bt_parse.h"
#include "peer.h"
#include "util.h"
#include "spiffy.h"
#include "congestion_control.h"

#define MAX_LINE_LENGTH 255
#define CHUNK_HASH_SIZE 41
#define MAGIC_NUMBER 15441
#define VERSION_NUMBER 1
#define HEADER_LENGTH 16
#define MAX_PACKET_SIZE 1500
#define MAX_PAYLOAD_SIZE 1484

#define CHUNK_NUMBER_AND_PADDING_SIZE 4

#define WHOHAS 0
#define IHAVE 1
#define GET 2
#define DATA 3
#define ACK 4
#define DENIED 5

#define CHUNK_PER_PACKET 74


struct packet{
    char* header;
    char payload[MAX_PAYLOAD_SIZE];
};

// print utilities
// void print_packet(struct packet* packet, unsigned short packet_length);

void fill_header(char* packet_header, unsigned char packet_type, unsigned short packet_length, unsigned int seq_number, unsigned int ack_number);

struct packet* make_packet(unsigned char packet_type, struct Chunk* p_chunk, char* data, int data_size, int seq_number, int ack_number, struct packet* packet_in, int* packet_count, struct Request* request);
void send_packet(struct packet packet, int socket, struct sockaddr* dst_addr);
void send_whohas_packet_to_all(struct packet* packets, int packet_count, int socket, struct sockaddr* dst_addr);
void whohas_flooding(struct Request* request);
int validate_packet(unsigned short magic_number, unsigned char version_number, unsigned char packet_type);

#endif