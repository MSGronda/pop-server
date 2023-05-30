include Makefile.inc
TARGET = server.out
INCDIR = ./include
MAIN = main.c

SRCDIRS = utils pop3 parsers metrics
SOURCES = $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
OBJS = $(SOURCES:.c=.o)

all: $(OBJS)
	cd utils; make all
	$(CC) $(CFLAGS) $(MAIN) -I$(INCDIR) $(OBJS) -o $(TARGET)

clean:
	cd utils; make clean
	rm -f $(TARGET)

.PHONY: all clean
