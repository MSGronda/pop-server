include ../Makefile.inc

INCLUDE = ./include
SOURCES = $(wildcard *.c)
OBJS = $(SOURCES:.c=.o)

all: $(OBJS)

%.o: %.c 
	$(CC) $(CFLAGS) -I$(INCLUDE) -c $<

clean:
	rm  -f *.o
	
