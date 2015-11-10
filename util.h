
#ifndef _UTIL_H_
#define _UTIL_H_

#include "common.h"

int find_chunk(char* hash);
struct Request* parse_has_get_chunk_file(char* chunk_file, char* output_filename);
void free_request(struct Request* p);
/* return timestamp in milliseconds */
unsigned long milli_time();

#endif

