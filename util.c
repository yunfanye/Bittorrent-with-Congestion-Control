int find_chunk(char* hash){
  int i = 0;
  for(i = 0; i < has_chunk_table->chunk_number; i++){
    if (memcmp(hash, has_chunk_table->chunks[i].hash, SHA1_HASH_SIZE) == 0) {
      return 1;
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
