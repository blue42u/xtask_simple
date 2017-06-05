CC = gcc -g
#CC = icc -mmic	#use this for to make an executable for Phi
CFLAGS = -std=c99 -D_GNU_SOURCE -O2 -Wall -Wpedantic -Werror -fPIC #-DVERBOSE
OBJECTS = basicqueue.o xtask_api.o worker.o
LDFLAGS = -lpthread

all: libxtask.a main

libxtask.a: $(OBJECTS)
	$(AR) rc libxtask.a $^

main: main.o libxtask.a
	$(CC) $(CFLAGS) $+ $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o main
