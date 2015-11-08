void send_one_whohas_ihave_packet(struct sockaddr* dst_addr, unsigned char packet_type, struct Chunk* chunks, unsigned char chunk_count, unsigned char offset){
  unsigned int packet_length = 20; // initial to header length
  unsigned char i, j;
  struct has_chunk* p = chunks;
  packet_length += chunk_count * SHA1_HASH_SIZE; // add chunk length
  char* packet = (char*)malloc(packet_length);
  fill_header(packet, packet_type, packet_length, 0, 0);
  *(unsigned char*)(packet+16) = chunk_count;
  i = 0;
  while(p!= NULL){
    if(i==offset){
      break;
    }
    i++;
    p = p->next;
  }
  j = 0;
  while(p!=NULL){
    memcpy(packet+20+SHA1_HASH_SIZE*j, p->hash, SHA1_HASH_SIZE);
    p = p->next;
    j++;
    if(j==chunk_count){
      break;
    }
  }
  assert(j==chunk_count);
  spiffy_sendto(sock, packet, packet_length, 0, dst_addr, sizeof(*dst_addr));
  // sendto(sock, packet, packet_length, 0, dst_addr, sizeof(*dst_addr));
  printf("sent one packet to %s:%d\n", inet_ntoa(((struct sockaddr_in*)dst_addr)->sin_addr),ntohs(((struct sockaddr_in*)dst_addr)->sin_port));
}

void send_whohas_ihave_packet(struct sockaddr* dst_addr, unsigned char packet_type, struct Chunk* chunks){
  unsigned int chunk_count = 0;
  unsigned int packet_count = 0;
  unsigned int j;
  struct has_chunk* p = chunks;
  while(p!= NULL){
    chunk_count++;
    p = p->next;
  }
  packet_count = chunk_count/CHUNK_PER_PACKET;
  if(packet_count==0){
    send_one_whohas_ihave_packet(dst_addr, packet_type, chunks, chunk_count, 0);
  }
  else{
    for(j=0;j<packet_count;j++){
      if(j!=packet_count-1){
        send_one_whohas_ihave_packet(dst_addr, packet_type, chunks, CHUNK_PER_PACKET, j*CHUNK_PER_PACKET);
      }
      else{
        if(packet_count*CHUNK_PER_PACKET<chunk_count){
          send_one_whohas_ihave_packet(dst_addr, packet_type, chunks, CHUNK_PER_PACKET, j*CHUNK_PER_PACKET);
          send_one_whohas_ihave_packet(dst_addr, packet_type, chunks, chunk_count-packet_count*CHUNK_PER_PACKET , (j+1)*CHUNK_PER_PACKET);
        }
        else if (packet_count*CHUNK_PER_PACKET==chunk_count){
          send_one_whohas_ihave_packet(dst_addr, packet_type, chunks, CHUNK_PER_PACKET, j*CHUNK_PER_PACKET);
        }
      }
    }
  }
}

void whohas_process(){
  chunk_count = *(unsigned char*)(buf+16);
  i=0;
  struct has_chunk* chunks = NULL;
  struct has_chunk* p_chunk = NULL;
  printf("WHOHAS packet received, chunk_count: %d, %d\n", chunk_count, i);
  while(i<chunk_count){
    if(find_chunk(buf+20+i*SHA1_HASH_SIZE, has_chunks)==1){
      p_chunk = (struct has_chunk*)malloc(sizeof(struct has_chunk));
      p_chunk->id = 0;
      memcpy(p_chunk->hash, buf+20+SHA1_HASH_SIZE*i, SHA1_HASH_SIZE);
      p_chunk->next = chunks;
      chunks = p_chunk;
    }
    i++;
  }
  if(chunks!=NULL){
    printf("Send back IHAVE packets, %d\n", i);
    send_whohas_ihave_packet((struct sockaddr *)&from, IHAVE, chunks);
  }
}