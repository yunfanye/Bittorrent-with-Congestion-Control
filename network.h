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
#include "spiffy.h"
#include "congestion_control.h"
#include "util.h"

// print utilities
void print_packet(struct packet* packet);
void print_incoming_packet(char* packet);

void fill_header(char** packet_header, unsigned char packet_type, unsigned short packet_length, unsigned int seq_number, unsigned int ack_number);

struct packet* make_packet(unsigned char packet_type, struct Chunk* p_chunk, char* data, int data_size, int seq_number, int ack_number, struct packet* packet_in, int* packet_count, struct Request* request);
void send_packet(struct packet packet, int socket, struct sockaddr* dst_addr);
void send_whohas_packet_to_all(struct packet* packets, int packet_count, int socket, struct sockaddr* dst_addr);
void whohas_flooding(struct Request* request);
int validate_packet(unsigned short magic_number, unsigned char version_number, unsigned char packet_type);

#endif