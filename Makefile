CC = gcc
LD = gcc
CC_OPTS = -O0 -Wall -g
LD_OPTS = $(CC_OPTS)

all:
	$(CC) $(CC_OPTS) -c src/*.c
	$(LD) $(LD_OPTS) -o chat_client *.o lib/*.a `pkg-config gtk+-2.0 --cflags --libs`

clean:
	rm -f *.o chat_client
