# Type 'make', 'make all' or 'make rebuild' to compile all files
# Type 'make (filename)' to compile only one file and create new executables
# Type 'make clean' to remove all object files
# Type 'make rebuild' to remove object files and create new executables
CC = gcc
FLAGS = -std=gnu99 -Wall
EXE = jms_console jms_coord pool
LINK1 = $(CC) $(FLAGS) -o jms_coord jms_coord.o pool_list.o job_list.o
LINK2 = $(CC) $(FLAGS) -o pool pool.o job_list.o

# Compile all files and create new executable files
all:
	$(CC) $(FLAGS) -c src/jms_console.c
	$(CC) $(FLAGS) -c src/jms_coord.c
	$(CC) $(FLAGS) -c src/pool.c
	$(CC) $(FLAGS) -c src/pool_list.c
	$(CC) $(FLAGS) -c src/job_list.c
	$(CC) $(FLAGS) -o jms_console jms_console.o
	$(CC) $(FLAGS) -o jms_coord jms_coord.o pool_list.o job_list.o
	$(CC) $(FLAGS) -o pool pool.o job_list.o

# Remove object files
clean:
	rm -f *.o
	rm -f *.fifo
	rm -f $(EXE)

# Remove object files, then compile all files and create new executable files
rebuild: clean all

# Compile each file separately and create new executable files
jms_console:
	$(CC) $(FLAGS) -c src/jms_console.c
	$(CC) $(FLAGS) -o jms_console jms_console.o

jms_coord:
	$(CC) $(FLAGS) -c src/jms_coord.c
	$(LINK1)

pool:
	$(CC) $(FLAGS) -c pool.c
	$(LINK2)

pool_list:
	$(CC) $(FLAGS) -c pool_list.c
	$(LINK1)

job_list:
	$(CC) $(FLAGS) -c job_list.c
	$(LINK1)
	$(LINK2)
