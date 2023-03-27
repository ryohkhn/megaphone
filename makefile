CC=gcc
CFLAGS= -Wall -g
LIBS= -lpthread

server : server.o
	$(CC) $(CFLAGS) -o server server.o $(LIBS)

server.o : server.c
	$(CC) $(CFLAGS) -c server.c -o server.o

client : client.o
	$(CC) $(CFLAGS) -o client client.o

client.o : client.c
	$(CC) $(CFLAGS) -c client.c -o client.o

clean :
	rm *.o
	rm server
	rm client
