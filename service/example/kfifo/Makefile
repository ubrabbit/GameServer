CC = gcc
CFLAGS = -std=c99 -Wall
LDFLAGS = -pthread

target = ./main

build: main.o ../../../common/kfifo/kfifo.o
	${CC} -o ${target} ${LDFLAGS} $^

run: build
	${target}

clean:
	# rm -rf *.o
	find . -name "*.o" | xargs rm -f
	rm -f ${target}
	rm -f ../../../common/kfifo/kfifo.o

all: clean build

.PHONY: all build clean run
