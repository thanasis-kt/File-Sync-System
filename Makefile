#Compile Options
CFLAGS =  -Wall -Werror -g 

# fss_manager arguments
EXEC_MANAGER_ARGS = -l manager_log.txt -c config_file.txt

# fss_console arguments
EXEC_CONSOLE_ARGS = -l console_log.txt 

# Files to be compiled (files without main)
OBJS = lnode.o queue.o sync_info_mem_store.o hash_table.o list_node.o 

# Our executable names
EXEC_MANAGER = fss_manager

EXEC_CONSOLE = fss_console

EXEC_WORKER = worker_process

$(EXEC_WORKER): $(OBJS) worker_process.o
	gcc $(OBJS) worker_process.o -o $(EXEC_WORKER) $(FLAGS)

$(EXEC_MANAGER): $(OBJS) fss_manager.o
	gcc $(OBJS) fss_manager.c -o $(EXEC_MANAGER) $(FLAGS)

$(EXEC_CONSOLE): $(OBJS) fss_console.o
	gcc $(OBJS)  fss_console.c -o $(EXEC_CONSOLE) $(FLAGS)

clean: 
	rm -f $(OBJS) $(EXEC_MANAGER) $(EXEC_CONSOLE) $(EXEC_WORKER) fss_console.o fss_manager.o worker_process.o

run: $(EXEC_MANAGER)  $(EXEC_WORKER) $(EXEC_CONSOLE)
	
	# We run fss_manager in the background
	./$(EXEC_MANAGER) $(EXEC_MANAGER_ARGS) & 

	# We run fss_console 
	./$(EXEC_CONSOLE) $(EXEC_CONSOLE_ARGS)
