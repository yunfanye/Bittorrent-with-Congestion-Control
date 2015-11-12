# Some variables
CC 		= gcc
CFLAGS		= -g -Wall -DDEBUG
LDFLAGS		= -lm
TESTDEFS	= -DTESTING			# comment this out to disable debugging code
OBJS		= window_log.o peer.o bt_parse.o spiffy.o debug.o input_buffer.o chunk.o sha.o keep_track.o network.o util.o file.o congestion_control.o
MK_CHUNK_OBJS   = make_chunks.o chunk.o sha.o

BINS            = peer make-chunks
TESTBINS        = test_debug test_input_buffer

# Implicit .o target
.c.o:
	$(CC) $(TESTDEFS) -c $(CFLAGS) $<

# Explit build and testing targets

all: ${BINS} ${TESTBINS}

run: peer_run
	./peer_run

test: peer_test
	./peer_test

peer: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

make-chunks: $(MK_CHUNK_OBJS)
	$(CC) $(CFLAGS) $(MK_CHUNK_OBJS) -o $@ $(LDFLAGS)

clean:
	rm -f *.o *~ $(BINS) $(TESTBINS)

# network.o: network.c network.h
# 	$(CC) $(CFLAGS) -c $^

# util.o: util.c util.h
# 	$(CC) $(CFLAGS) -c $^

# keep_track.o: keep_track.c keep_track.h
# 	$(CC) $(CFLAGS) -c $^

bt_parse.c: bt_parse.h

# The debugging utility code

debug-text.h: debug.h
	./debugparse.pl < debug.h > debug-text.h

test_debug.o: debug.c debug-text.h
	${CC} debug.c ${INCLUDES} ${CFLAGS} -c -D_TEST_DEBUG_ -o $@

test_input_buffer:  test_input_buffer.o input_buffer.o

handin:
	(make clean; git commit -a -m "handin"; git tag -d checkpoint-1; git tag -a checkpoint-1 -m "handin"; cd ..; tar cvf project2.tar 15-441-project-2)
