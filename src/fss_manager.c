#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/inotify.h>
#include <assert.h>
#include "fss_manager.h"
#include "lnode.h"
#include "sync_info_mem_store.h"
#include "queue.h"
#include "hash_table.h"

// The permissions of the fifos we will create
#define PERMS 0666
// The pipe stream we will create
#define READ 0
#define WRITE 1
// For the pollfd array
#define INOTIFY 0
#define FSS_IN 1

int main(int argc,char* argv[]) {
    if (argc > 7) { // If it's <= 7 we check for the arguments bellow  
        fprintf(stderr,"ERROR <%s>!Wrong number of arguments given",argv[0]);
        exit(-1);
    }

    // Creating our named pipes 
    if ((mkfifo("fss_in",PERMS)) < 0) {
        if (errno == EEXIST) {
            unlink("fss_in");
            if ((mkfifo("fss_in",PERMS)) < 0) {
                perror("ERROR! fss_in couldn't be created\n");
                exit (-1);
            }
        }
        else {
            perror("ERROR! fss_in couldn't be created\n");
            exit(-1);
        }
    }
    if ((mkfifo("fss_out",PERMS)) < 0) {
        if (errno == EEXIST) {
            unlink("fss_out");
            if ((mkfifo("fss_out",PERMS)) < 0) {
                perror("ERROR! fss_out couldn't be created\n");
                unlink("fss_in");
                exit (-1);
            }
        }
        else {
            perror("ERROR! fss_out couldn't be created\n");
            exit(-1);
        }
    }
    char* logfile = "";
    char* config_file = "";
    int worker_limit = DEFAULT_WORKERS_LIMIT;
    for (int i = 1; i < argc; i++) {
        // Every time we read a correct prefix, we increase i by 1, that
        //  way we can guarantee that every comparison we make will be either 
        //  -l,-c or -n (1 exec_file + 3 comparisons + 3 increases = 7)
        
        // Check if we have already assigned the below values
        if (strcmp(argv[i],"-l") == 0 && logfile[0] == '\0') {
            logfile = argv[++i];
        }
        else if (strcmp(argv[i],"-c") == 0 && config_file[0] == '\0') {
            config_file = argv[++i];
        }
        else if (!strcmp(argv[i],"-n")) { 
            worker_limit = atoi(argv[++i]);
            if (worker_limit == 0) {
                fprintf(stderr,"ERROR <%s>! Wrong number of arguments given\n",argv[0]);
                exit(-1);
            }
        }
        else {
            fprintf(stderr,"ERROR <%s>! Wrong type of arguments given\n",argv[0]);
            exit(-1);
        }
    }
    // Add signal responce
    signal(SIGCHLD,handler);
    int inotify_fd;
    if ((inotify_fd = inotify_init()) == -1) {
        perror("ERROR! inotify_init failed\n");
        exit(-1);
    } 
    sync_info_mem_store* mem = sync_info_mem_store_create();
    // Now we will read from config file and synchronize the directories
    FILE* config_input = fopen(config_file, "rb");
    FILE* logfile_output = fopen(logfile,"w"); // or w+?
    if (config_input == NULL || logfile_output == NULL) {
        perror("ERROR! fopen failed\n");
        exit(-1);
    }
    hash_table* inotify_map = hash_table_create();
    queue* workers_queue = queue_create();
    int num_of_current_workers = 0; 
    // Our array that will be used for monitoring pipes for I/O
    struct pollfd* fds_array = malloc(sizeof(struct pollfd) * worker_limit);
    // Every pipe will have it's own buffer
    char** workers_buffer = malloc(sizeof(char*) * worker_limit);
    if (workers_buffer == NULL) {
        perror("ERROR! malloc failed\n");
        exit(-1);
    }
    for (int i = 0; i < worker_limit; i++) {
        workers_buffer[i] = malloc(sizeof(char) * 1024);
        if (workers_buffer[i] == NULL) {
            perror("ERROR! malloc failed\n");
            exit(-1);
        }
        workers_buffer[i][0] = '\0';
    }
    pid_t* workers_pid = malloc(sizeof(pid_t) * worker_limit);
    if (workers_pid == NULL) {
        perror("ERROR! malloc failed\n");
        exit(-1);
    }
    // Buffer that will be used to track and print timestamps
    char time_buffer[32];


    char buffer[1024];
    while (feof(config_input) == 0 || num_of_current_workers > 0 || queue_is_empty(workers_queue) == false) {
        char s2[100];
        char* source_dir;
        char* target_dir;
        // Checking if any worker has written it's work and flushing it to it's buffer
        if (poll(fds_array,num_of_current_workers,0) > 0) {
            int n;
            for (int i = 0; i < num_of_current_workers; i++) {
                if ((fds_array[i].revents & POLLIN) != 0) {
                    n = read(fds_array[i].fd,buffer,1024);
                    if (n < 0) {
                        perror("ERROR! read failed\n");
                        exit(-1);
                    }
                    buffer[n] = '\0';
                    strcat(workers_buffer[i],buffer);
                    // Worker finished 
                    if (is_substring(workers_buffer[i],"EXEC_REPORT_END"))  {
                        // Print in the logfile
                        fprintf(logfile_output,"[%s] ",print_timestamp(time_buffer));
                        int errors_found = 0;
                        char *status,*operation,*details;
                        extract_workers_output(workers_buffer[i],&source_dir,&target_dir,&status,&operation,&details,&errors_found);
                        fprintf(logfile_output,"[%s] [%s] [%d] [%s] [%s] [%s]\n",source_dir,target_dir,workers_pid[i],operation,status,details);
                        dir_info* info = sync_info_mem_store_find(mem,source_dir);
                        if (info == NULL) {
                            fprintf(stderr,"ERROR info for <%s> not found\n",source_dir);
                            exit(-1);
                        }
                        info->is_in_workers = false;
                        info->active_error_count += errors_found;
                        strcpy(info->last_sync_time,time_buffer);
                        strcpy(info->status,status);
                        // We place the buffer and pipe to the end so they can
                        // be replaced by another worker
                        workers_buffer[i][0] = '\0';

                        int pid_temp = workers_pid[i];
                        workers_pid[i] = workers_pid[num_of_current_workers - 1];
                        workers_pid[num_of_current_workers - 1] = pid_temp;

                        struct pollfd temp = fds_array[i];
                        fds_array[i] = fds_array[num_of_current_workers - 1];
                        fds_array[num_of_current_workers - 1] = temp;

                        char* tmp = workers_buffer[i];
                        workers_buffer[i] = workers_buffer[num_of_current_workers - 1];
                        workers_buffer[num_of_current_workers] = tmp;

                        num_of_current_workers--;
                    }
                }

            }
        }
        if (num_of_current_workers < worker_limit && queue_is_empty(workers_queue) == false){
            strcpy(buffer,queue_pop_action(workers_queue));
        }
        else {
            strcpy(buffer,"");
            fscanf(config_input,"%s",buffer);
            int n = fscanf(config_input,"%s",s2);
            if (n < 1) {
                fscanf(config_input,"%s",buffer);
                continue;
            }
            strcat(buffer," ");
            strcat(buffer,s2);
            strcat(buffer," ");
        }
        if (num_of_current_workers < worker_limit)  {
            source_dir = strtok(buffer," \n");
            target_dir = strtok(NULL," \n");
            // Create two processes with a pipe to communicate
            int fd_to_worker[2];
            int watch_fd; // To store watch fd from inotify 
            pipe(fd_to_worker);
            int child_pid;
            switch ((child_pid =fork())) {
                case -1:
                    perror("ERROR! fork failed\n");
                    exit(-1);
                // Child Process
                case 0:
                    // Redirect stdout to pipe and use exec* to launch a worker
                    close(fd_to_worker[READ]);
                    close(1); //Closing stdout
                    dup2(fd_to_worker[WRITE],1);
                    execl("worker_process","worker_process" ,source_dir,target_dir,"ALL","FULL",NULL);
                    // If this line gets executed, exec failed for some reason
                    perror("ERROR! execl failed\n");
                    exit(-1);

                // Parent process
                default:
                    // Add source_dir to inotify and save it on sync_info_mem_store
                    watch_fd = inotify_add_watch(inotify_fd, source_dir, IN_CREATE | IN_DELETE | IN_MODIFY );
                    hash_table_insert(inotify_map, watch_fd,source_dir); 
                    fprintf(logfile_output,"[%s] Monitoring started for %s\n",print_timestamp(time_buffer),source_dir);
                    dir_info new_dir;
                    new_dir.is_active = true; // We start monitoring the directory pair
                    new_dir.is_in_workers = true; // We don't have to specify a sync 
                    strcpy(new_dir.source_dir,source_dir);
                    strcpy(new_dir.target_dir,target_dir);
                    // last_sync_time and status will be assigned after 
                    //  worker finishes
                    strcpy(new_dir.last_sync_time,""); 
                    strcpy(new_dir.status,""); 
                    new_dir.active_error_count = 0;
                    sync_info_mem_store_add(mem,new_dir);
                    // Parent doesn't write in the pipe 
                    close(fd_to_worker[WRITE]);
                    workers_pid[num_of_current_workers] = child_pid;
                    // Add pipe to pollfd structure 
                    fds_array[num_of_current_workers].fd = fd_to_worker[READ];
                    fds_array[num_of_current_workers].events = POLLIN;
                    num_of_current_workers++;
                    break;
            }


        }
        else {
            // push to queue the action add source_dir target_dir
            queue_push_action(workers_queue,buffer);
        }
           
    }
    // Now we will use inotify to check for changes in files and fss_in
    //  to take commands from fss_console
    // We will use poll to check fss_in and inotify descriptors.
     // Opening our fifo's
    int in_fd,out_fd;
    if ((in_fd = open("fss_in",O_RDONLY)) < 0)  {
        perror("ERROR! failed to open read fifo");
        exit(-1);
    }
    if ((out_fd = open("fss_out",O_WRONLY)) < 0)  {
        perror("ERROR! failed to open write fifo");
        exit(-1);
    }
    num_of_current_workers = 0;

    struct pollfd* fds_for_input = malloc(sizeof(struct pollfd) * 2);
    if (fds_for_input == NULL) {
        perror("ERROR! malloc failed\n");
        exit(-1);
    }

    fds_for_input[INOTIFY].fd = inotify_fd;
    fds_for_input[INOTIFY].events = POLLIN;
    fds_for_input[FSS_IN].fd = in_fd;
    fds_for_input[FSS_IN].events = POLLIN;
    bool shutdown_flag = false;
    int pos = 0;
    do {
        bool workers_flag = false; //Is true when we need to create a worker
        char *target_dir,*source_dir,*filename,*action;
        // A curent worker has ended
        if (poll(fds_array,num_of_current_workers,0) > 0) {
            int n;
            for (int i = 0; i < num_of_current_workers; i++) {
                if ((fds_array[i].revents & POLLIN) != 0) {
                    n = read(fds_array[i].fd,buffer,1024);
                    if (n < 0) {
                        perror("ERROR! read failed\n");
                        exit(-1);
                    }
                    buffer[n] = '\0';
                    strcat(workers_buffer[i],buffer);
                    // Worker finished 

                    if (is_substring(workers_buffer[i],"EXEC_REPORT_END"))  {
                        // Print to logfile
                        fprintf(logfile_output,"[%s] ",print_timestamp(time_buffer));
                        int errors_found = 0;
                        char *status,*operation,*details;
                        extract_workers_output(workers_buffer[i],&source_dir,&target_dir,&status,&operation,&details,&errors_found);
                        fprintf(logfile_output,"[%s] [%s] [%d] [%s] [%s] [%s]\n",source_dir,target_dir,workers_pid[i],operation,status,details);
                        dir_info* info = sync_info_mem_store_find(mem,source_dir);
                        if (info == NULL) {
                            fprintf(stderr,"ERROR info for <%s> not found\n",source_dir);
                            exit(-1);
                        }
                        if (!strcmp(operation,"FULL")) {
                            if (strcmp(status,"ERROR")) {
                                dprintf(out_fd,"[%s] Sync completed %s -> %s\n",print_timestamp(time_buffer),source_dir,target_dir);
                            }
                            else {
                                dprintf(out_fd,"[%s] Sync %s -> %s coudn't complete\n",print_timestamp(time_buffer),source_dir,target_dir);
                            }
                            info->is_in_workers = false;
                        }
                        info->active_error_count += errors_found;
                        strcpy(info->last_sync_time,time_buffer);
                        strcpy(info->status,status);
                        // We place the buffer and pipe to the end so they can
                        // be replaced by another worker
                        workers_buffer[i][0] = '\0';

                        int pid_temp = workers_pid[i];
                        workers_pid[i] = workers_pid[num_of_current_workers - 1];
                        workers_pid[num_of_current_workers - 1] = pid_temp;

                        struct pollfd temp = fds_array[i];
                        fds_array[i] = fds_array[num_of_current_workers - 1];
                        fds_array[num_of_current_workers - 1] = temp;

                        char* tmp = workers_buffer[i];
                        workers_buffer[i] = workers_buffer[num_of_current_workers - 1];
                        workers_buffer[num_of_current_workers] = tmp;

                        num_of_current_workers--;
                    }

                }

            }
        }
             
        if (num_of_current_workers < worker_limit && queue_is_empty(workers_queue) == false) {
            strcpy(buffer,queue_pop_action(workers_queue));
            workers_flag = true;
        }                                 //
        else if (poll(fds_for_input,2,0) > 0 || pos > 0) {
            // Read input and create a new worker process

            if (fds_for_input[INOTIFY].revents == POLLIN || pos > 0) {
                //TODO Make sure if more than one reads we don't lose information
                int n = read(fds_for_input[INOTIFY].fd,buffer + pos,1024);
                struct inotify_event* inotify_value = (struct inotify_event*)buffer;
                pair* pr = hash_table_find(inotify_map, inotify_value->wd);
                if (pr == NULL) {
                    fprintf(stderr,"ERROR! source directory with watch fd <%d> didn't found",inotify_value->wd);
                    exit(-1);
                }
                // We read more than one inotify instance
                //  We need to re-read remaining data after completing this working process task
                if (strlen(buffer) > sizeof(struct inotify_event) + inotify_value->len + 1) {
                    buffer[n] = '\0';
                    pos = sizeof(struct inotify_event) + inotify_value->len + 1 - n;
                    strcpy(buffer,buffer + sizeof(struct inotify_event) + inotify_value->len + 1);
                }
                else {
                    pos = 0;
                }
                source_dir = pr->value;
                filename = inotify_value->name;
                dir_info* dr = sync_info_mem_store_find(mem,source_dir);
                assert(dr != NULL);
                target_dir = dr->target_dir;
                workers_flag = true;
                if ((inotify_value->mask & IN_CREATE) != 0) {
                    action = "ADDED";
                }
                else if ((inotify_value->mask & IN_MODIFY) != 0) {
                    action = "MODIFIED";
                }
                else if ((inotify_value->mask & IN_DELETE) != 0) {
                    action = "DELETED";
                }
                else {
                    continue;
                }
            }
            else if (fds_for_input[FSS_IN].revents == POLLIN){ //fss_in has input
                char fifo_buffer[1024];
                int n;
                buffer[0] = '\0';
                do {
                    n = read(fds_for_input[FSS_IN].fd,fifo_buffer,1024);
                    fifo_buffer[n] = '\0';
                    strcat(buffer,fifo_buffer);
                } while (is_full_command(fifo_buffer) == false);
                // Constructing the command 
                action = strtok(buffer," \n");
                if (!strcmp(action,"add")) {
                    source_dir = strtok(NULL," \n");
                    target_dir = strtok(NULL," \n");
                    bool is_active; // To know if the directory is active
                    // Add pair to mem if we haven't used this source_dir before
                    dir_info* dir_value = sync_info_mem_store_find(mem,source_dir);
                    if (dir_value == NULL) {
                        dir_info new_value;
                        strcpy(new_value.source_dir,source_dir);
                        strcpy(new_value.target_dir,target_dir);
                        strcpy(new_value.last_sync_time,"");
                        new_value.active_error_count = 0;
                        new_value.is_active = true;
                        is_active = false;
                        sync_info_mem_store_add(mem,new_value);
                    }
                    else {
                        is_active = dir_value->is_active;
                        if (is_active == false) {
                            dir_value->is_active = true;
                            strcpy(dir_value->target_dir,target_dir);
                        }
                    }
                    if (is_active == false) {
                        int watch_fd = inotify_add_watch(inotify_fd, source_dir, IN_CREATE | IN_DELETE | IN_MODIFY);
                        fprintf(logfile_output,"[%s] Monitoring started for %s\n",print_timestamp(time_buffer),source_dir);
                        hash_table_insert(inotify_map,watch_fd, source_dir);
                        dprintf(out_fd,"[%s] Added directory %s -> %s\n",print_timestamp(time_buffer),source_dir,target_dir);
                        // Also create a worker to sync the directories 
                        workers_flag = true;
                        action = "FULL";
                        filename = "all";
                    }    
                    else {
                        dprintf(out_fd,"[%s] File is already monitored\n",print_timestamp(time_buffer));
                    }
                    
                }
                else if (!strcmp(action,"status")) {
                    source_dir = strtok(NULL," \n");
                    dir_info* status_info = sync_info_mem_store_find(mem, source_dir);
                    if (status_info == NULL) {
                        dprintf(out_fd,"[%s] Directory %s not monitored\n",print_timestamp(time_buffer),source_dir);    
                    }
                    else {
                        dprintf(out_fd,"[%s] Status requested for %s\n",print_timestamp(time_buffer),source_dir);
                        dprintf(out_fd,"\tDirectory: %s\n",source_dir);
                        dprintf(out_fd,"\tTarget: %s\n",status_info->target_dir);
                        dprintf(out_fd,"\tIs Active: %s\n",(status_info->is_active) ? "yes" : "no");
                        dprintf(out_fd,"\tLast Sync:%s\n",status_info->last_sync_time);
                        dprintf(out_fd,"\tErrors: %d\n",status_info->active_error_count);
                        dprintf(out_fd,"\tStatus: %s\n",status_info->status);
                    }    

                }
                else if (!strcmp(action,"sync")) {
                    source_dir = strtok(NULL," \n");
                    dir_info* info = sync_info_mem_store_find(mem, source_dir);
                    if (info == NULL) {
                        dprintf(out_fd,"[%s] Directory %s not monitored\n",print_timestamp(time_buffer),source_dir);
                    }
                    else {
                        // Check that we can sync the directory (is monitored and we are not trying to sync it)
                        if (info->is_in_workers == true) {
                            dprintf(out_fd,"[%s] Sync already in progress %s\n",print_timestamp(time_buffer),source_dir);
                        }
                        else if (info->is_active == false) {
                            dprintf(out_fd,"[%s] Directory %s not monitored\n",print_timestamp(time_buffer),source_dir);
                        }
                        // We can sync it
                        else {
                            workers_flag = true;
                            target_dir = info->target_dir;
                            filename = "all";
                            action = "FULL";
                            int watch_fd = inotify_add_watch(inotify_fd, source_dir, IN_CREATE | IN_DELETE | IN_MODIFY);
                            hash_table_insert(inotify_map,watch_fd, source_dir);

                            //Write message to fifo and logfile
                            dprintf(out_fd,"[%s] Syncing directory: %s -> %s\n",print_timestamp(time_buffer),source_dir,target_dir);
                            fprintf(logfile_output,"[%s] Syncing directory: %s -> %s\n",print_timestamp(time_buffer),source_dir,target_dir);
                        }    
                    }    
                }
                else if (!strcmp(action,"cancel")) {
                    source_dir = strtok(NULL," \n");
                    int wd = hash_table_find_key(inotify_map,source_dir);
                    if (wd != -1) {
                        dir_info* dr = sync_info_mem_store_find(mem,source_dir);
                        if (dr->is_active == true) {
                            dr->is_active = false;
                            inotify_rm_watch(inotify_fd,wd);
                            dprintf(out_fd,"[%s] Monitoring stopped for %s\n",print_timestamp(time_buffer),source_dir);
                            fprintf(logfile_output,"[%s] Monitoring stopped for %s\n",print_timestamp(time_buffer),source_dir);
                        }   
                        else {
                            dprintf(out_fd,"[%s] Directory not monitored\n",print_timestamp(time_buffer));
                        }
                    }
                    else
                        dprintf(out_fd,"[%s] Directory not monitored\n",print_timestamp(time_buffer));
                }
                else if (!strcmp(action,"shutdown")) {
                    dprintf(out_fd,"[%s] Shuting down manager...\n",print_timestamp(time_buffer));
                    dprintf(out_fd,"[%s ]Waiting for all active workers to finish.\n",print_timestamp(time_buffer));
                    shutdown_flag = true;
                }
                else {
                    dprintf(out_fd,"Wrong command given\n");
                }
            
            }
            if (shutdown_flag == true && num_of_current_workers == 0) {
                dprintf(out_fd,"[%s] Processing remaining queued tasks.\n",print_timestamp(time_buffer));
            }

            if (workers_flag == true && num_of_current_workers < worker_limit) {
                int fd_to_worker[2];
                pipe(fd_to_worker);
                int child_pid;
                switch ((child_pid = fork())) {
                    case -1:
                        perror("ERROR! fork failed\n");
                        exit(-1);
                    // Worker process
                    case 0:
                        // Redirect stdout to pipe and use exec* to launch a worker
                        close(fd_to_worker[READ]);
                        close(1); // To redirect pipe to it
                        dup2(fd_to_worker[WRITE],1);
                        execl("worker_process","worker_process",source_dir,target_dir,filename,action,NULL);
                        // If this line gets executed, exec failed for some reason
                        perror("ERROR! execl failed\n");
                        exit(-1);
                    default:
                        workers_pid[num_of_current_workers] = child_pid;
                        close(fd_to_worker[WRITE]);
                        fds_array[num_of_current_workers].fd = fd_to_worker[READ];
                        fds_array[num_of_current_workers].events = POLLIN;
                        num_of_current_workers++;
                        break;
                }
            }
            else if (workers_flag) {
                // push action to queue 
                char action_queue[1024];
                strcpy(action_queue,source_dir);
                strcat(action_queue," ");
                strcat(action_queue,target_dir);
                strcat(action_queue," ");
                strcat(action_queue,filename);
                strcat(action_queue," ");
                strcat(action_queue,action);
                queue_push_action(workers_queue,action_queue);
            }
        }
    } while(shutdown_flag == false || queue_is_empty(workers_queue) == false || num_of_current_workers > 0); // Shutdown will halt the program
    dprintf(out_fd,"[%s] Manager shutdown complete\n",print_timestamp(time_buffer));
    fclose(logfile_output); 
    close(out_fd);
    close(in_fd);
}


// SIGCHLD handler that will be used to deal with zombie processes
void handler(int sig) {
    // We wait for child to finish
    int status;
    wait(&status);
}

bool is_substring(char* str, char* sub) {
    int i = 0;
    int j = 0;
    while (str[i] != '\0') {
        if (str[i] == sub[j]) {
            j++;
            if (sub[j] == '\0')
                return true;
             
        }
        else 
            j = 0;
       i++;
    }
    return false;
}

// Returns true if buffer contains '\n' character. Buffer must be NULL 
// terminated
bool is_full_command(char* buffer) {
    int i = 0;
    while (buffer[i] != '\0') {
        if (buffer[i] == '\n')
            return true;
        i++;
    }
    return false;
}

char* print_timestamp(char time_buffer[32]) {
    time_t tm;
    struct tm* time_info;
    time(&tm);
    time_info = localtime(&tm);
    strftime(time_buffer, 32, "%Y-%m-%d %H:%M:%S", time_info);
    return time_buffer;
}


void extract_workers_output(char* buffer,char** source_dir,char** target_dir,char** status,char** operation,char** details,int* errors_found) {
    // We will use : and \n as delimeters to manipulate our input
    char* str = strtok(buffer,":\n");
    *errors_found = 0;
    while (str != NULL) {
        if (!strcmp(str,"SOURCE DIR")) {
            // The next one is our source directory (we 
            //  also trim whitespace)
            str = strtok(NULL," :\n");
            *source_dir = str; // We keep this to update mem_sync
        }
        else if (!strcmp(str,"TARGET DIR")) {
            // The next one is our target directory
            str = strtok(NULL," :\n");
            *target_dir = str;
        }
        else if (!strcmp(str,"STATUS")) {
            str = strtok(NULL," :\n");
            *status = str;
        }
        else if (!strcmp(str,"OPERATION")) {
            // The next one is the operation performed
            str = strtok(NULL," :\n");
            *operation = str;
        }
        else if (!strcmp(str,"DETAILS")) {
            // The next one is our details
            str = strtok(NULL,":\n"); // we keep whitespace in string
            *details = str;
        }
        else if (!strcmp(str,"ERRORS")) {
            // We want to count the errors to put them in sync_info
            // We want to view strings: - File <filename>: error\n as one
            str = strtok(NULL,"\n");
            // Until we read exec_report_end
            while (strcmp(str,"EXEC_REPORT_END")) {
                (*errors_found)++;
                str = strtok(NULL,"\n");
            }
        }
        str = strtok(NULL,":\n");
    }
}
