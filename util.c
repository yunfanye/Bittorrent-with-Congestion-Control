int find_chunk(char* hash){
  int i = 0;
  for(i = 0; i < has_chunk_table->chunk_number; i++){
    if (memcmp(hash, has_chunk_table->chunks[i].hash, SHA1_HASH_SIZE) == 0) {
      return 1;
    }
  }
  return -1;
}

int get_chunk_id(char* hash, struct Request* chunk_table){
  int i = 0;
  for(i = 0; i < chunk_table->chunk_number; i++){
    if (memcmp(hash, chunk_table->chunks[i].hash, SHA1_HASH_SIZE) == 0) {
      return i;
    }
  }
  return -1;
}

// parse has/get chunk file and return request if output_filename != NULL
// if called by get request with a output_filename, then chunk status is set to NOT_STARTED
// if called during peer initilization with output_filename=NULL, then chunk status is set to OWNED
struct Request* parse_has_get_chunk_file(char* chunk_file, char* output_filename){
  FILE *f;
  int chunk_count = 0;
  int i;
  char hash[SHA1_HASH_SIZE*2];
  char binary_hash[SHA1_HASH_SIZE];
  char line[MAX_LINE_LENGTH];
  struct Chunk* p_chunk;

  f = fopen(chunk_file, "r");
  while (fgets(line, MAX_LINE_LENGTH, f) != NULL) {
    if (line[0] == '#'){
      continue;
    }
    chunk_count++;
  }
  fseek(chunk_file, 0, SEEK_SET);

  struct Request* request = (struct Request*)malloc(sizeof(struct Request));
  request->filename = NULL;
  request->chunk_number = chunk_count;
  request->chunks = (struct Chunk*)malloc(sizeof(struct Chunk) * chunk_count);
  p_chunk = request->chunks;
  i = 0;
  while (fgets(line, MAX_LINE_LENGTH, f) != NULL) {
    sscanf(line,"%d %s", &(p_chunk[i].id), hash);
    hex2binary(hash, SHA1_HASH_SIZE*2, (uint8_t*)binary_hash);
    strncpy(p_chunk[i].hash, binary_hash, sizeof(binary_hash));
    if(output_filename!=NULL){
      p_chunk[i].state = NOT_STARTED;
    }
    else{
      p_chunk[i].state = OWNED; 
    }
  }
  fclose(f);

  if(output_filename!=NULL){
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

char* pick_a_chunk(struct packet* packet, struct Chunk** chunk_pointer){
  int i=0;
  struct Chunk* p_chunk=current_request->chunks;
  for(i=0;i<current_request->chunk_number;i++){
    if(p_chunk[i].state==NOT_STARTED && packet_contain_chunk(packet, p_chunk[i].hash)==1){
      p_chunk[i].state = RECEIVING;
      *chunk_pointer = p_chunk[i];
      return p_chunk[i].hash;
    }
  }
  *chunk_pointer = NULL;
  return NULL;
}

int packet_contain_chunk(struct packet* packet, char* hash){
  unsigned char chunk_count = *(unsigned char*)(packet+16);
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
  struct Chunk* chunk = current_request->chunks[chunk_id];
  unsigned int seq_number = *(unsigned int*)((char*)in_packet+8);
  unsigned short header_length = *(unsigned short*)((char*)in_packet+4);
  unsigned short packet_length = *(unsigned short*)((char*)in_packet+6);
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
  struct Chunk* chunk = current_request->chunks[chunk_id];
  if(chunk->received_byte_number == BT_CHUNK_SIZE){
    chunk -> state = OWNED;
    // verify chunk
    char hash[SHA1_HASH_SIZE];
    shahash((char*)chunk->data, BT_CHUNK_SIZE, hash);
    // if chunk verify failed
    if (memcmp(hash, chunk->hash_binary, SHA1_HASH_SIZE) != 0){
      chunk -> state = NOT_STARTED;
    }
    else{
      filename = current_request->filename;
      write_file(filename, chunk->data, BT_CHUNK_SIZE, offset);
    }
    free(chunk->data);
    chunk->data = NULL;
    chunk->received_seq_num = 0;
    chunk->received_byte_num = 0;
  }
}

int all_chunk_finished(){
  for(i=0;i<current_request->chunk_number;i++){
    if(p_chunk[i].state!=OWNED)}{
      return 0;
    }
  }
  return 1;
}