#ifndef _UTIL_H_
#define _UTIL_H_

#include "common.h"
#include "sha.h"
#include "chunk.h"
#include "peer.h"
#include "file.h"
#include <sys/time.h>
#include <assert.h>

int find_chunk(uint8_t* hash);
struct Request* parse_has_get_chunk_file(char* chunk_file, char* output_filename);
void free_request(struct Request* p);
/* return timestamp in milliseconds */
unsigned long milli_time();
int save_chunk(int chunk_id);
void save_data_packet(struct packet* in_packet, int chunk_id);
int packet_contain_chunk(struct packet* packet, uint8_t* hash);
uint8_t* pick_a_chunk(struct packet* packet, struct Chunk** chunk_pointer);
int get_chunk_id(uint8_t* hash, struct Request* chunk_table);
int all_chunk_finished();
uint8_t* pick_a_new_chunk(int peer_id, struct Chunk** chunk_pointer);
int peer_contain_chunk(int peer_id, uint8_t* hash);
struct Chunk* update_chunks(struct Chunk* chunks, int* chunk_count, struct packet* packet);
void update_connections(int peer_id, struct packet* incoming_packet);

void print_connection(struct connection* connection);
void print_connections();
void print_request(struct Request* request);
void print_chunks(struct Chunk* chunks, int chunk_number);
#endif

