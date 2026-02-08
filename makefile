CC = gcc
CFLAGS = -Wall -Wextra -O2 -lm

all: backend server

backend: backend.c kdtree.h
	$(CC) $(CFLAGS) -DCLI_MODE backend.c -o backend

server: server.c backend.c kdtree.h
	$(CC) $(CFLAGS) server.c backend.c -o server -lpthread

clean:
	rm -f backend server *.o

run-backend:
	./backend

run-server:
	./server

.PHONY: all clean run-backend run-server