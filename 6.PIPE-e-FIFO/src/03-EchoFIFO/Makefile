CC = gcc -Wall -g

all: echo client

echo: echo.c common.h rw.c
	$(CC) -o echo echo.c

client: client.c common.h rw.c
	$(CC) -o client client.c

.PHONY: clean

clean:
	rm -f echo client fifo_client fifo_echo
