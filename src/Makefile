include Makefile.inc
TARGET = server.out
INCDIR = ./include
MAIN = main.c
ARGS = args.c

SRCDIRS = utils pop3 parser metrics
SOURCES = $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
OBJS = $(SOURCES:.c=.o)

all: $(OBJS)
	cd utils; make all
	cd pop3; make all
	cd parser; make all
	$(CC) $(CFLAGS) $(MAIN) $(ARGS) -I$(INCDIR) $(OBJS) -o $(TARGET)

clean:
	cd utils; make clean
	cd pop3; make clean
	cd parser; make clean
	rm -f $(TARGET)

.PHONY: all clean
