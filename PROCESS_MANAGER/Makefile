#!/bin/sh

CC			= gcc
CFLAG       = -g -W -Wall -Wno-unused -m64 -fno-strict-aliasing

LOC_INC		= -I. 

SRCS		= mo_main.c 

OBJS		= $(SRCS:.c=.o)

AOUT		= mosnd

#---------------------------------------------------------------

.c.o:
	$(CC) $(CFLAG) -c $<

$(AOUT): $(OBJS)
	$(CC) $(CFLAG) -o $(AOUT) $(OBJS)

clean:
	rm -f $(AOUT) $(OBJS)
