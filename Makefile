# env
CC ?= gcc

DEBUG ?= 0

ifeq ($(CC),clang)
    CFLAGS_WARN = -Weverything
else
    CFLAGS_WARN = -Wall -Wextra -pedantic
endif

ARCH=$(shell uname -m)

ifeq ($(DEBUG),1)
CFLAGS += -O0 -g3 -march=native -std=gnu99 $(CFLAGS_WARN) -fsanitize=address
LDFLAGS += -lasan
else
CFLAGS += -O3 -march=native -std=gnu99 $(CFLAGS_WARN) -DNDEBUG
endif

ifeq ($(ARCH), x86_64)
SOURCES = $(wildcard sse_*.c)
else ifeq ($(ARCH), aarch64)
SOURCES = $(wildcard neon_*.c)
else
$(error invalid ARCH)
endif
SOURCES += estimate_clock_frequency.c

OBJECTS = $(SOURCES:.c=.o)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

build: $(OBJECTS)
	$(foreach obj, $(OBJECTS), $(CC) $(CFLAGS) $(LDFLAGS) -o $(basename $(obj)) $(obj);)

clean:
	-rm -vf $(OBJECTS)
	-$(foreach obj, $(OBJECTS), rm -vf $(basename $(obj));)


.DEFAULT_GOAL := build
