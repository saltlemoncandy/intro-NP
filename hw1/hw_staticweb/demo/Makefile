CC		= gcc
PROGS	= server_https

all: server_https

%.o: %.c
	$(CC) -c -g -Wall server_https

server_https: server_https.o
	$(CC) -o $@ -static $^ -lssl -lcrypto

clean:
	rm -f *.o $(PROGS)
