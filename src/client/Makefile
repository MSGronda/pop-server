include ../Makefile.inc

SRC = client.c
TARGET = client.out
INCLUDE = ./include
SOURCES = $(wildcard *.c)
MNG_LIB = ../server/m3.o

all:
	$(CC) $(CFLAGS) $(SRC)  $(MNG_LIB) -I$(INCLUDE) -o $(TARGET)

clean:
	rm  -f *.o
	
