
CROSS_COMPILE=arm-none-linux-gnueabi-
CC=$(CROSS_COMPILE)gcc
CPP=$(CC) -E
LD=$(CROSS_COMPILE)ld
AS=$(CROSS_COMPILE)as
MAKE=make

all: discovery cmd

discovery:discovery.o 
	$(CC) -o discovery discovery.o $(CFLAGS) -lpthread -lm $(LIBS) 
discovery.o: discovery.c
	$(CC) -c discovery.c $(CFLAGS)

cmd:cmd.o 
	$(CC) -o cmd cmd.o $(CFLAGS) -lpthread -lm $(LIBS) 
cmd.o: cmd.c
	$(CC) -c cmd.c $(CFLAGS)
	
clean:
	rm -rf *.o discovery cmd *~



