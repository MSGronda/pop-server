include ../Makefile.inc

SRC = client.c
TARGET = client.out
INCLUDE = ./include
SOURCES = $(wildcard *.c)

all:
	$(CC) $(CFLAGS) $(SRC) -I$(INCLUDE) -o $(TARGET)

clean:
	rm  -f *.o
	
