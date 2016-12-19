#
# CMPSC311 - F16 Assignment #4
# Makefile - makefile for the assignment
#

# Locations

# Make environment
INCLUDES=-I. 
CC=gcc
CFLAGS=-I. -c -g -Wall $(INCLUDES)
LINKARGS=-g
LIBS=-lm -lcmpsc311 -L. -lgcrypt -lpthread -lcurl
                    
# Suffix rules
.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS)  -o $@ $<
	
# Files

CLIENT_FILES=	cart_sim.o \
				cart_client.o \
				cart_driver.o \
				cart_cache.o \

# Productions
all : cart_client

cart_client : $(CLIENT_FILES)
	$(CC) $(LINKARGS) $(CLIENT_FILES) -o $@ $(LIBS)

clean : 
	rm -f cart_client $(CLIENT_FILES)
