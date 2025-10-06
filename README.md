# File Sync System

This repo is a file sync system implemented as a project
for a Systems Programming course in my university. The pdf description of the 
assignment is also included in this github repo.

## Program Description

We are using 4 different programs to implement the functionality of the file sync
system:

- worker_process 
- fss_console
- fss_manager

### worker_process 

This program is a process created by fss_manager and it communicates with it using
pipes and signals. A worker process syncs two files by either adding or deleting files 
from a source directory to a target directory. They are executed by fss_manager using 
`exec` and do a specific syncing task and then terminate

### fss_console

fss_console is the program that communicates with the user and fss_manager. It 
reads commands from the user through stdin, and sends them to fss_manager 
through a FIFO pipe. After fss_manager executes the given command, it sends it's 
output to fss_console and nsf_console prints it to the user. 

Executing fss_console:

`./fss_console -l <console-logfile> 

- <console-logfile>: A logfile that fss_console writes all the given commands.

We can enter the following commands:

- add <source> <target>: Adds a directory pair for synchronization.
- status <directory>: Returns info for the syncing status of a directory
- cancel <source>: Cancels the syncing of <source> and it's pair. It leaves
pending syncing tasks.
- shutdown: Shuts down fss_manager and terminates.


### fss_manager    

fss_manager is the core of the fss system. It communicates with fss_console 
through pipes, and creates worker processes to satisfy the syncing requests
it receives. It also uses inotify calls to monitor it's directories, and 
ensure that all pairs are synced, in case of modification,deletion or addition
of a file.

It is executed as:

./fss_manager -l <manager_logfile> -c <config_file> -n <worker_limit>

- manager_logfile: The logfile of fss_manager
- config_file: a file containing pairs of directories, that fss_manager syncs
before interacting with fss_console.
- worker_limit: the number of worker processes that can be running at the same
time (default 5). If we can't create another worker, we push the syncing requests in a 
queue.



### fss_script.sh

This bash script is used for the two following functions:

- Purging a diretory 

- Listing directories that are actively monitored, or stopped being monitored,
or even every directory that have been monitored.

It is executed as:

`./fss_script.sh -p <path> -c <command>`

- command can be listAll,listMonitored,listStopped or purge
- path is a logfile (from fss_manager) or a directory (if the option is purge).

### Compilation 

A makefile is included in the repo. To compile just run `make <program_name>`, 
or if you wan to execute them run `make run`. In the second case be sure
of the given parameters in the makefile.



## Notes

This was implemented as a university project, so it was required to use a lot 
of syscalls in some areas. So you may come across a whole bunch of write calls 
instead of a printf.

This project was built using git controll, but it was in a private repository
(in github classroom) and i couldn't publish it with it's previous git history.
So i republish it here as a public repository


