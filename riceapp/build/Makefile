CC = gcc
CFLAGS = -I../include -Wall -O2
LDFLAGS = -lm
SRCDIR = ../src
OBJDIR = ../obj
BUILDDIR = ../build
TARGET = cube_app

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(BUILDDIR)/$(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJDIR)/*.o $(BUILDDIR)/$(TARGET)

.PHONY: all clean

