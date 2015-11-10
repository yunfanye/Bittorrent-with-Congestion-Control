
#ifndef _UTIL_H_
#define _UTIL_H_

#include "common.h"
#include "connection.h"
#include "network.h"

int find_chunk(char* hash);
struct Request* parse_has_get_chunk_file(char* chunk_file, char* output_filename);
void free_request(struct Request* p);
/* return timestamp in milliseconds */
unsigned long milli_time();
void save_chunk(int chunk_id);
void save_data_packet(struct packet* in_packet, int chunk_id);
int packet_contain_chunk(struct packet* packet, char* hash);
char* pick_a_chunk(struct packet* packet, struct Chunk** chunk_pointer);
int get_chunk_id(char* hash, struct Request* chunk_table);
int all_chunk_finished();
char* pick_a_new_chunk(int peer_id, struct Chunk** chunk_pointer);
int peer_contain_chunk(int peer_id, char* hash);
struct Chunk* update_chunks(struct Chunk* chunks, int* chunk_count, struct packet* packet);
void update_connections(int peer_id, struct packet* incoming_packet);
void print_connection(struct connection* connection);

#endif

