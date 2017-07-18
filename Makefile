#CC = gcc -O3
#CC = icc -mmic -O3	#use this for to make an executable for Phi
CC = clang -g #-O3
CFLAGS = -std=gnu99 -Wall -fPIC -Iinclude/
LDFLAGS = -lpthread

all: libxtask-counter.a libxtask-oneatom.a \
	libxtask-jigstack.a libxtask-atomstack.a \
	xtask.so

libxtask-%.a: xdata.o xtask.o %.o
	$(AR) rc $@ $^

xtask.so: lua.o libxtask-jigstack.a
	$(CC) $^ -shared -o $@ $(LDFLAGS)

%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *.a *.so
