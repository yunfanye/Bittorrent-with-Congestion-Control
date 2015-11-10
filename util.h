
#ifndef _UTIL_H_
#define _UTIL_H_

#include "common.h"
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



#endif

