CC=gcc
CFLAGS=-O2 -Wall -I.

all: benchmark

benchmark: benchmark.c mock_esp_idf.h
	$(CC) $(CFLAGS) -o benchmark benchmark.c

clean:
	rm -f benchmark
