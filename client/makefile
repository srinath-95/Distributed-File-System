#makefile#

all: client

client: client.c
	gcc -Wall client.c -o client -lcrypto -lssl -lm

clean:
	rm -f client
