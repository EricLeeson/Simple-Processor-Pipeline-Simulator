## This is a simple Makefile

# Define what compiler to use and the flags.
CC=cc
CXX=CC
CCFLAGS= -g -std=c99 -Wall -Werror


all: proj

# Compile all .c files into .o files
# % matches all (like * in a command)
# $< is the source file (.c file)
%.o : %.c
	$(CC) -c $(CCFLAGS) $<


proj: simulation.o
	$(CC) -o proj simulation.o -lm


clean:
	rm -f *.o proj
