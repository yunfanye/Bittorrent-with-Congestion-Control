#include "util.h"

int find_chunk(uint8_t* hash){
  int i = 0;
  for(i = 0; i < has_chunk_table->chunk_number; i++){
    if (memcmp(hash, has_chunk_table->chunks[i].hash, SHA1_HASH_SIZE) == 0) {
      return 1;
    }
  }
  return -1;
}

int get_chunk_id(uint8_t* hash, struct Request* chunk_table){
  int i = 0;
  for(i = 0; i < chunk_table->chunk_number; i++){
    if (memcmp(hash, chunk_table->chunks[i].hash, SHA1_HASH_SIZE) == 0) {
      return i;
    }
  }
  return -1;
}

void print_request(struct Request* request){
  if(request==NULL){
    printf("NULL request\n");
    return;
  }
  printf("Filename: %s, chunk_number: %d\n", request->filename, request->chunk_number);
  print_chunks(request->chunks, request->chunk_number);
}

void print_chunks(struct Chunk* chunks, int chunk_number){
  int i=0;
  char hash_buffer[SHA1_HASH_SIZE * 2 + 1];
  for (i = 0; i < chunk_number; ++i)
  {
    memset(hash_buffer, 0, SHA1_HASH_SIZE * 2 + 1);
    binary2hex(chunks[i].hash, SHA1_HASH_SIZE, hash_buffer);
    printf("id: %d Hash: %s state: %d\n", chunks[i].id, hash_buffer, chunks[i].state);
  }
}

// parse has/get chunk file and return request if output_filename != NULL
// if called by get request with a output_filename, then chunk status is set to NOT_STARTED
// if called during peer initilization with output_filename=NULL, then chunk status is set to OWNED
struct Request* parse_has_get_chunk_file(char* chunk_file, char* output_filename){
  FILE *f;
  int chunk_count = 0;
  int i;
  uint8_t hash[SHA1_HASH_SIZE*2+1];
  uint8_t binary_hash[SHA1_HASH_SIZE];
  char line[MAX_LINE_LENGTH];
  struct Chunk* p_chunk;
  f = fopen(chunk_file, "r");
  assert(f != NULL);
  while (fgets(line, MAX_LINE_LENGTH, f) != NULL) {
    if (line[0] == '#'){
      continue;
    }
    chunk_count++;
  }
  fseek(f, 0, SEEK_SET);
  struct Request* request = (struct Request*)malloc(sizeof(struct Request));
  request->filename = NULL;
  request->chunk_number = chunk_count;
  request->chunks = (struct Chunk*)malloc(sizeof(struct Chunk) * chunk_count);
  p_chunk = request->chunks;
  i = 0;
  //int tempnum;
  while (fgets(line, MAX_LINE_LENGTH, f)) {
    sscanf(line, "%d %s", &(p_chunk[i].id), hash);
    hex2binary((char*)hash, SHA1_HASH_SIZE*2, binary_hash);
    strncpy((char*)(p_chunk[i].hash), (char*)binary_hash, sizeof(binary_hash));
    if(output_filename!=NULL){
      p_chunk[i].state = NOT_STARTED;
    }
    else{
      p_chunk[i].state = OWNED; 
    }
    p_chunk[i].received_seq_number = 0;
    p_chunk[i].received_byte_number = 0;
    p_chunk[i].data = NULL;
    i++;
  }
  fclose(f);
  if(output_filename!=NULL){
    request->filename = (char*)malloc(FILE_NAME_SIZE);
    memcpy(request->filename, output_filename, FILE_NAME_SIZE);
  }
  return request;
}

void free_request(struct Request* p){
  free(p->chunks);
  free(p);
}

/* return timestamp in milliseconds */
unsigned long milli_time() {
	struct timeval time;
	gettimeofday(&time, NULL);
	return time.tv_sec * 1000 + time.tv_usec;
}

uint8_t* pick_a_chunk(struct packet* packet, struct Chunk** chunk_pointer){
  int i=0;
  struct Chunk* p_chunk=current_request->chunks;
  for(i=0;i<current_request->chunk_number;i++){
    if(p_chunk[i].state==NOT_STARTED && packet_contain_chunk(packet, p_chunk[i].hash)==1){
      p_chunk[i].state = RECEIVING;
      *chunk_pointer = &p_chunk[i];
      return p_chunk[i].hash;
    }
  }
  *chunk_pointer = NULL;
  return NULL;
}

uint8_t* pick_a_new_chunk(int peer_id, struct Chunk** chunk_pointer){
  int i=0;
  struct Chunk* p_chunk=current_request->chunks;
  for(i=0;i<current_request->chunk_number;i++){
    if(p_chunk[i].state==NOT_STARTED && peer_contain_chunk(peer_id, p_chunk[i].hash)==1){
      p_chunk[i].state = RECEIVING;
      *chunk_pointer = &p_chunk[i];
      return p_chunk[i].hash;
    }
  }
  *chunk_pointer = NULL;
  return NULL;
}

int peer_contain_chunk(int peer_id, uint8_t* hash){
  struct connection* temp = connections;
  int i=0;
  while(temp){
    if(temp->id==peer_id){
      for(i=0;i<temp->chunk_count;i++){
        if(memcmp(hash, temp->chunks[i].hash, SHA1_HASH_SIZE) == 0){
          return 1;
        }
      }
    }
  }
  return -1;
}

int packet_contain_chunk(struct packet* packet, uint8_t* hash){
  unsigned char chunk_count = *(unsigned char*)((char*)packet+16);
  unsigned char i = 0;
  char* packet_chunk = NULL;
  for(i=0;i<chunk_count;i++){
    packet_chunk = (char*)(packet+20+20*i);
    if(memcmp(hash, packet_chunk, SHA1_HASH_SIZE) == 0){
      return 1;
    }
  }
  return -1;
}

// save data in the packet
void save_data_packet(struct packet* in_packet, int chunk_id){
  struct Chunk* chunk = &current_request->chunks[chunk_id];
  unsigned int seq_number = ntohl(*(unsigned int*)((char*)in_packet+8));
  unsigned short header_length = ntohs(*(unsigned short*)((char*)in_packet+4));
  unsigned short packet_length = ntohs(*(unsigned short*)((char*)in_packet+6));
  int data_size = packet_length - header_length;
  if(data_size < 0){
    data_size = 0;
  }
  if(data_size > MAX_PAYLOAD_SIZE){
    data_size = MAX_PAYLOAD_SIZE;
  }
  if(chunk->data == NULL){
    chunk->data = malloc(sizeof(char) * BT_CHUNK_SIZE);
  }
  chunk->received_seq_number = seq_number;
  chunk->received_byte_number = chunk->received_byte_number + data_size;
  memcpy(chunk->data + chunk->received_byte_number, in_packet->payload, data_size);
}

void save_chunk(int chunk_id){
  char* filename = NULL;
  struct Chunk* chunk = &current_request->chunks[chunk_id];
  if(chunk->received_byte_number == BT_CHUNK_SIZE){
    chunk -> state = OWNED;
    // verify chunk
    uint8_t hash[SHA1_HASH_SIZE];
    shahash((uint8_t*)chunk->data, BT_CHUNK_SIZE, hash);
    // if chunk verify failed
    if (memcmp(hash, chunk->hash, SHA1_HASH_SIZE) != 0){
      chunk -> state = NOT_STARTED;
    }
    else{
      int offset = chunk->id * BT_CHUNK_SIZE;
      filename = current_request->filename;
      write_file(filename, chunk->data, BT_CHUNK_SIZE, offset);
    }
    free(chunk->data);
    chunk->data = NULL;
    chunk->received_seq_number = 0;
    chunk->received_byte_number = 0;
  }
}

int all_chunk_finished(){
  if(current_request==NULL){
    return 0;
  }
  int i=0;
  for(i=0;i<current_request->chunk_number;i++){
    if(current_request->chunks[i].state!=OWNED){
      return 0;
    }
  }
  return 1;
}

void update_connections(int peer_id, struct packet* incoming_packet){
  if(connections==NULL){
    connections = (struct connection*)malloc(sizeof(struct connection));
    connections->id = peer_id;
    connections->chunks = NULL;
    connections->chunks = update_chunks(connections->chunks, &(connections->chunk_count), incoming_packet);
    connections->next = NULL;
  }
  else{
    struct connection* temp = connections;
    while(temp){
      if(temp->id==peer_id){
        // update peer's chunk table
        temp->chunks = update_chunks(temp->chunks, &(temp->chunk_count), incoming_packet);
        return;
      }
      temp = temp->next;
    }
    // connections does not contain peer
    temp = (struct connection*)malloc(sizeof(struct connection));
    temp->id = peer_id;
    temp->chunks = NULL;
    temp->chunks = update_chunks(temp->chunks, &(temp->chunk_count), incoming_packet);
    temp->next = connections;
    connections = temp;
  }
  print_connections();
}

// completely delete previous chunk table and create new one
struct Chunk* update_chunks(struct Chunk* chunks, int* chunk_count, struct packet* packet){
  unsigned short packet_length = ntohs(*(unsigned short*)((char*)packet+6));
  printf("update_chunks: %hu\n", packet_length);
  int i=0;
  char* temp_hash = NULL;
  free(chunks);
  *chunk_count = (packet_length-20)/20;
  chunks = (struct Chunk*)malloc(sizeof(struct Chunk)*(*chunk_count));
  for(i=0;i<*chunk_count;i++){
    temp_hash = (char*)packet + 20 + i * 20;
    strncpy((char*)(chunks[i].hash), temp_hash, 20);
  }
  return chunks;
}

void print_connection(struct connection* connection){
  printf("Peer id: %d, Chunks: %d\n", connection->peer_id, connection->chunk_count);
  int i=0;
  char hash_buffer[SHA1_HASH_SIZE * 2 + 1];
  for(i=0;i<connection->chunk_count;i++){
    memset(hash_buffer, 0, SHA1_HASH_SIZE * 2 + 1);
    binary2hex(connection->chunks[i].hash, SHA1_HASH_SIZE, hash_buffer);
    printf("Hash: %s\n", hash_buffer);
  }
}

void print_connections(){
  printf("Begin printing all connections.......................................\n");
  struct connection* temp = connections;
  while(temp){
    print_connection(temp);
    temp = temp->next;
  }
  printf("Finish printing all connections.......................................\n");
}
