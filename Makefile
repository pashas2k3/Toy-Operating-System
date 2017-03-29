CC = gcc
CFLAGS=-g -Wall -Wextra -pedantic

EXECS = myOS

all: $(EXECS)

clean: 
	rm -f *.o $(EXECS)
	echo Clean done

memory_manager.h : memory_manager.c
file_system.h : file_system.c
ledManipulation.h: ledManipulationTest.c
utility.h: utility.c memory_manager.c shell.c ledManipulationTest.c file_system.c

memory_manager.o: memory_manager.c 
utility.o: utility.c 
file_system.o : file_system.c memory_manager.h
shell.o: shell.c memory_manager.h file_system.h 
ledManipulationTest.o: ledManipulationTest.c ledManipulation.h

$(EXECS): shell.o memory_manager.o ledManipulationTest.o file_system.o utility.o
	  $(CC) $(CFLAGS) -o $@ $^
