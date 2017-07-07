#CC = gcc -O3
#CC = icc -mmic -O3	#use this for to make an executable for Phi
CC = clang -O3
CFLAGS = -std=gnu99 -Wall
LDFLAGS = -lpthread

all: libxtask-jigstack.a libxtask-counter.a libxtask-oneatom.a

libxtask-%.a: xtask.o %.o
	$(AR) rc $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o libxtask-*.a
