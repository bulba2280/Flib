CC      ?= cc
AR      ?= ar
CFLAGS  ?= -O2 -g
CFLAGS  += -std=c11 -Wall -Wextra -Wpedantic -Iinclude
PREFIX  ?= /usr/local

BUILD  := build
LIB_A  := $(BUILD)/libflib.a
LIB_SO := $(BUILD)/libflib.so
SRC    := $(wildcard src/*.c)
OBJ    := $(patsubst src/%.c,$(BUILD)/%.o,$(SRC))

.PHONY: all install uninstall clean docs

all: $(LIB_A) $(LIB_SO)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: src/%.c include/flib.h src/internal.h | $(BUILD)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(LIB_A): $(OBJ)
	$(AR) rcs $@ $^

$(LIB_SO): $(OBJ)
	$(CC) -shared -o $@ $^

install: all
	install -d $(DESTDIR)$(PREFIX)/include $(DESTDIR)$(PREFIX)/lib
	install -m 644 include/flib.h $(DESTDIR)$(PREFIX)/include/
	install -m 644 $(LIB_A) $(DESTDIR)$(PREFIX)/lib/
	install -m 755 $(LIB_SO) $(DESTDIR)$(PREFIX)/lib/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/include/flib.h
	rm -f $(DESTDIR)$(PREFIX)/lib/libflib.a
	rm -f $(DESTDIR)$(PREFIX)/lib/libflib.so

docs:
	python3 docs/build.py

clean:
	rm -rf $(BUILD)
