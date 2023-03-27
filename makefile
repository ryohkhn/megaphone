CC=gcc
CFLAGS= -Wall -g
LIBS= -lpthread

server : server.o utilities.o
	$(CC) $(CFLAGS) -o server server.o utilities.o $(LIBS)

server.o : server.c server.h
	$(CC) $(CFLAGS) -c server.c -o server.o

client : client.o utilities.o
	$(CC) $(CFLAGS) -o client client.o utilities.o

client.o : client.c client.h
	$(CC) $(CFLAGS) -c client.c -o client.o

utilities.o : utilities.c utilities.h
	$(CC) $(CFLAGS) -c utilities.c -o utilities.o

clean :
	rm *.o
	rm server
	rm client
