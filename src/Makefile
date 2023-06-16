include Makefile.inc
TARGET = server.out
INCDIR = ./include
MAIN = main.c

SRCDIRS = utils pop3 parser server
SOURCES = $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
OBJS = $(SOURCES:.c=.o)

all: $(OBJS)
	cd utils; make all
	cd pop3; make all
	cd parser; make all
	cd server; make all
	cd client; make all
	$(CC) $(CFLAGS) $(MAIN) -I$(INCDIR) $(OBJS) -o $(TARGET)

clean:
	cd utils; make clean
	cd pop3; make clean
	cd parser; make clean
	cd server; make clean
	cd client; make clean
	rm -f $(TARGET)

.PHONY: all clean
