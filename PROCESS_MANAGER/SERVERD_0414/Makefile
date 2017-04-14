#!/bin/sh

CC			= gcc
CFLAG       = -g -W -Wall -Wno-unused -m64 -fno-strict-aliasing

LOC_INC		= -I. 

SRCS		= serverd_main.c serverd_init.c serverd_socket.c serverd_queue.c

OBJS		= $(SRCS:.c=.o)

AOUT		= serverd

#---------------------------------------------------------------

.c.o:
	$(CC) $(CFLAG) -c $<

$(AOUT): $(OBJS)
	$(CC) $(CFLAG) -o $(AOUT) $(OBJS)

clean:
	rm -f $(AOUT) $(OBJS)
