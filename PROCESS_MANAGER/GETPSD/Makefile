#!/bin/sh

CC			= gcc
CFLAG       = -g -W -Wall -Wno-unused -m64 -fno-strict-aliasing

LOC_INC		= -I. 

SRCS		= psd_main.c psd_init.c psd_socket.c

OBJS		= $(SRCS:.c=.o)

AOUT		= getpsd

#---------------------------------------------------------------

.c.o:
	$(CC) $(CFLAG) -c $<

$(AOUT): $(OBJS)
	$(CC) $(CFLAG) -o $(AOUT) $(OBJS)

clean:
	rm -f $(AOUT) $(OBJS)
