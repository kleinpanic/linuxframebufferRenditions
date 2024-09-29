# build/Makefile
CC = gcc
CFLAGS = -Wall -I../include
SRCDIR = ../src
OBJDIR = ../obj
BINDIR = ../build
TARGET = clock

# Gather all source files in src directory
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(BINDIR)/$(TARGET) $(OBJECTS) -lm

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJDIR)/*.o $(BINDIR)/$(TARGET)

.PHONY: all clean
