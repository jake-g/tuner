CC = gcc
CFLAGS = -O3 -Wall -Wextra -Werror -std=c99 -D_XOPEN_SOURCE
CFLAGS += $(shell pkg-config portaudio-2.0 --cflags)
LDFLAGS = $(shell pkg-config portaudio-2.0 --libs-only-L --libs-only-other)
LIBS = $(shell pkg-config portaudio-2.0 --libs-only-l)

TARGET = tuner
SRC_DIR = src
OBJ_DIR = objs
PREFIX ?= /usr/local

CFILES = $(wildcard $(SRC_DIR)/*.c)
HEADERS = $(wildcard $(SRC_DIR)/*.h)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(CFILES))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
	rm -rf $(OBJ_DIR)

install: $(TARGET)
	mkdir -p $(PREFIX)/bin
	cp -f $(TARGET) $(PREFIX)/bin/$(TARGET)
	chmod 755 $(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)

help:
	@echo "Tuner Makefile Targets:"
	@echo "  make          Compile binary ($(TARGET))"
	@echo "  make run      Compile and run tuner application"
	@echo "  make clean    Remove compilation artifacts"
	@echo "  make install  Install binary globally to $(PREFIX)/bin"
	@echo "  make uninstall Remove binary from $(PREFIX)/bin"

.PHONY: all run clean install uninstall help
