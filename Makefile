CC = gcc -g
#CC = icc -mmic	#use this for to make an executable for Phi
CFLAGS = -std=c99 -D_GNU_SOURCE -O2 -Wall -Wpedantic -fPIC #-DVERBOSE
OBJECTS = basicstack.o xtask_api.o
LDFLAGS = -lpthread

all: libxtask.a main fib

libxtask.a: $(OBJECTS)
	$(AR) rc libxtask.a $^

main: main.o libxtask.a
	$(CC) $(CFLAGS) $+ $(LDFLAGS) -o $@

fib: fib.o libxtask.a
	$(CC) $(CFLAGS) $+ $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o main fib
