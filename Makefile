CC = gcc
CFLAGS = -fPIC -Wall
AR = ar
LFALGS = -lpthread

SO_TARGET = liblocalq.so
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

.c.o:
	$(CC) $(CFLAGS) -c $<


localq: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LFALGS)

#liblocalq.a: $(OBJS)
#	$(AR) -r $@ $^

clean:
	rm -rf $(OBJS)

.PHONY: client
client:
	@(cd client; $(MAKE))
