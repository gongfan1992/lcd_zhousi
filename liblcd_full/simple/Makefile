# Default target.
all:

CC = gcc

OPT = -Os
CFLAGS = -std=gnu99 -Wall -g $(OPT) 
LIBS = -Wl,-Bstatic -L../build -llcd -Wl,-Bdynamic -L/usr/lib64

SRCS = testapp.c

OBJS = $(SRCS:%.c=%.o)

$(OBJS): %.o : %.c
	$(CC) $(CFLAGS) $(XFLAGS) -c -o $@ $<

testapp: testapp.o
	$(CC) $(LDFLAGS) $(XFLAGS) -o $@ $^ $(LIBS) -lpthread

all: testapp
	@cp testapp ../build
	

clean:
	rm -f *.o *.d testapp

.PHONY: all clean
