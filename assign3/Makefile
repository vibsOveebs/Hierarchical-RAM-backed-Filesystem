#
# CMPSC311 - F16 Assignment #2
# Makefile - makefile for the assignment
#

# Locations
CMPSC311_LIBDIR=.

# Make environment
INCLUDES=-I. -I$(CMPSC311_LIBDIR)
CC=gcc
CFLAGS=-I. -c -g -Wall $(INCLUDES)
LINKARGS=-g
LIBS=-lcartlib -lcmpsc311 -lgcrypt -lcurl -L$(CMPSC311_LIBDIR) 
                    
# Suffix rules
.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS)  -o $@ $<
	
# Files
OBJECT_FILES=	cart_sim.o \
				cart_driver.o \
                cart_cache.o
				
# Productions
all : cart_sim

cart_sim : $(OBJECT_FILES)
	$(CC) $(LINKARGS) $(OBJECT_FILES) -o $@ $(LIBS)

clean : 
	rm -f cart_sim $(OBJECT_FILES)
	
