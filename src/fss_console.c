#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>
#include "../include/fss_console.h"
#define PERMS 0666
// For the poll_fd array
#define FSS_OUT 0
#define STDIN 1

int main(int argc,char* argv[])  {
    if (argc != 3) {
        fprintf(stderr,"ERROR <%s>! Wrong number of parameters given\n",argv[0]);
        exit(-1);
    }
    //Wrong type of argument (we only accept ./fss_console -l <file>)
    if (strcmp(argv[1],"-l")) {
        fprintf(stderr,"ERROR <%s>! Wrong parameters given\n",argv[0]);
        exit(-1);
    }
    FILE* logfile = fopen(argv[2],"w");
    if (logfile == NULL) {
        perror("ERROR! Couldn't open console_log file\n");
        exit(-1);
    }
    int fss_in_fd = open("fss_in",O_WRONLY);
    int fss_out_fd = open("fss_out",O_RDONLY);
    if (fss_in_fd < 0 || fss_out_fd < 0) {
        perror("ERROR! opening fifo failed\n");
        exit(-1);
    }
    // Variables that will be used to track and print timestamps
    time_t tm;
    struct tm* time_info;
    char time_buffer[32]; // To print timestamps in form ["%Y-%m-%d %H:%M:%S"]
    // Input and output buffers
    char output[1024];
    char action[1024];
    char source_dir[1024];
    char target_dir[1024];
    source_dir[0] = '\0';
    target_dir[0] = '\0';
    struct pollfd* io_array = malloc(sizeof(struct pollfd) * 2);
    io_array[FSS_OUT].fd = fss_out_fd;
    io_array[FSS_OUT].events = POLLIN;
    io_array[STDIN].fd = 0;
    io_array[STDIN].events = POLLIN;
    printf("Enter your commands:\n ");
    do {
        if (poll(io_array,2,0) > 0) {
            if (io_array[FSS_OUT].revents == POLLIN) {
                // Now we read fss_console's output from the fss_out and print it 
                // to the user
                int n;
                int pos = 0;

                n = read(fss_out_fd,output + pos,1024);
                if (n < 0) {
                    perror("ERROR! failed to read from fifo");
                    exit(-1);
                }
                pos += n;
                output[pos] = '\0'; 
                printf("%s\n",output);           
            }
            else if (io_array[STDIN].revents == POLLIN){
                // We keep making them empty because not all commands require them
                source_dir[0] = '\0';
                target_dir[0] = '\0'; 
                scanf("%s",action);
                if (!strcmp(action,"add")) {
                    scanf("%s",source_dir);
                    scanf("%s",target_dir);
                }
                else if (!strcmp(action,"status")) {
                    scanf("%s",source_dir);
                }
                else if (!strcmp(action,"cancel")) {
                    scanf("%s",source_dir);
                }
                else if (!strcmp(action,"sync")) {
                    scanf("%s",source_dir);
                }       
                // We entered the wrong command
                else if (strcmp(action,"shutdown")) {
                    printf("Wrong command given\n");
                    continue;
                }

                // Write input in fifo
                dprintf(fss_in_fd,"%s %s %s\n",action,source_dir,target_dir);
                // Write input in logfile with a timestamp
                time(&tm);
                time_info = localtime(&tm);
                strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", time_info);
                fprintf(logfile,"[%s] Command %s %s %s\n",time_buffer,action,source_dir,target_dir);
            }
        }        
    } while (strcmp(action,"shutdown")); // shutdown command halts the loop
    // Now that the program is preparing to shutdown. We read until our special
    // character is given, in order to not miss any data  
    int n;
    do {
        n = read(fss_out_fd,output,1024);
        // The pipe is closed for some reason
        if (n < 0) {
            break;
        }
        write(1,output,n);
    } while (n > 0);
    // Program exits successfully
    return 0;
}
