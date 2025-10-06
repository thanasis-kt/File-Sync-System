/* Header file that describes the functionality of a worker process. A worker 
 * process is responcible for synchronizing a file/directory with it's source.
 *It uses low-level I/O and communicates with it's parent process (fss_manager)
 *  through a pipe it takes as an argument. When a worker process terminates, it 
 *  sends a SIGCHLD signal to parent.
 *
 *  A worker process takes the following parameters:
 *      - source_directory
 *      - target_directory
 *      - filename
 *      - operation: {FULL,ADDED,MODIFIED,DELETED}
 *
 *  The program writes in stdout (fss_console redirects it to a pipe)
* */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>


extern int errno; // For error handling messages


void sync_full();
void sync_added_file();
void sync_modified_file();

void sync_deleted_file();


char* number_to_string(char* buffer,int n);

/* This function synchronizes the following two files by copying from the first
 * file to the second one: 
 *      - source_dir/filename
 *      - target_dir/filename 
 *
 * It returns 0 if it was successfull or -1 if an error 
 * happend 
 **/
int synchronize_files(char* source_dir,char* target_dir,char* filename);


/* This function deletes the file target_dir/filename
 * It returns 0 if it was successfull or -1 if an error 
 * happend 
 * */
int delete_file(char* target_dir,char* filename);


