#!/bin/sh

CC			= gcc
CFLAG       = -g -W -Wall -Wno-unused -m64 -fno-strict-aliasing

LOC_INC		= -I. 

SRCS		= psm_main.c psm_init.c psm_queue.c

OBJS		= $(SRCS:.c=.o)

AOUT		= psmanager

#---------------------------------------------------------------

.c.o:
	$(CC) $(CFLAG) -c $<

$(AOUT): $(OBJS)
	$(CC) $(CFLAG) -o $(AOUT) $(OBJS)

clean:
	rm -f $(AOUT) $(OBJS)
