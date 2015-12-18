# This is a -*- Makefile -*-

# Default target.
all:

CC = gcc
AR = ar

RM = rm -f
MKDIR = mkdir -p

INCLUDES =  
EXTRA_CFLAGS ?=

CFLAGS = -std=gnu99 -O2 -g -ffunction-sections -Wall -Wshadow -Werror\
  -D_GNU_SOURCE $(INCLUDES) $(EXTRA_CFLAGS)

DLL_FLAGS = \
  -Wl,--gc-sections \
  -Wl,--fatal-warnings \
  -Wl,--warn-unresolved-symbols


SOURCES := \
  liblcd.c \
  testapp.c

-include $(SOURCES:.c=.d)
LIBS = -L. -llcd -lpthread

%.o: %.c
	$(CC) $(CFLAGS) -MD -MP -fpic -c -o $@ $<

testapp: testapp.o
	$(CC) $(LDFLAGS) $(XFLAGS) -o $@ $^ $(LIBS)

liblcd.a: liblcd.o 
	rm -f $@
	$(AR) cq $@ $^

all: liblcd.a testapp

clean:
	$(RM) *.d  *.o lib*.a testapp

.PHONY: all clean

