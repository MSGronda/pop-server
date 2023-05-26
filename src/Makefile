CC = gcc
CFLAGS = g --std=c11 -fsanitize=address  -Wall -Wextra -Werror -Wno-unused-parameter -Wno-implicit-fallthrough-Wall
TARGET = your_program
OBJDIR = obj
INCDIR = include

SRCDIRS = metrics persers pop3 utils

SOURCES := $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
OBJECTS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

vpath %.c $(SRCDIRS)


all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR)/$(dir $<) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)


.PHONY: all clean