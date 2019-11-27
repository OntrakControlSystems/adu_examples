CFLAGS := -O2 -std=c99 -Wall

adu_example: adu_example.o
	$(CC) -o adu_example adu_example.o -lusb-1.0

default: adu_example

clean:
	rm adu_example adu_example.o
