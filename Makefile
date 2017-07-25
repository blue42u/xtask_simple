#CC = gcc -O3
#CC = icc -mmic -O3	#use this for to make an executable for Phi
CC = clang -O3
CFLAGS = -std=gnu99 -Wall -fPIC -Iinclude/
LDFLAGS = -lpthread

all: libxtask-counter.a libxtask-jigstack.a xtask.so

libxtask-%.a: build/xdata.o build/xtask.o build/%.o
	$(AR) rc $@ $^

xtask.so: build/lua.o build/ldata-write.o build/ldata-read.o libxtask-jigstack.a
	$(CC) $^ -shared -o $@ $(LDFLAGS)

build/%.o: src/%.c build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir build

clean:
	rm -rf build *.a *.so
