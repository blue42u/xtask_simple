CC = gcc -g
#CC = icc -mmic	#use this for to make an executable for Phi
CFLAGS = -std=c99 -D_GNU_SOURCE -O2 -Wall -Wpedantic -fPIC #-DVERBOSE
OBJECTS = jigstack.o xtask.o
LDFLAGS = -lpthread

all: libxtask-jigstack.a libxtask-basicstack.a

libxtask-%.a: xtask.o %.o
	$(AR) rc $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o libxtask-*.a
