/* Source file that implements a worker process. A worker process is 
 * used to synchronize a target file/directory with it's source. It uses 
 * low-level I/O and writes in standar output (stdout). When a worker process 
 *  terminates, it sends a SIGCHLD signal to it's parent process.
 *
 *  A worker process takes the following parameters:
 *      - source_directory
 *      - target_directory
 *      - filename
 *      - operation: {FULL,ADDED,MODIFIED,DELETED}
 *
 *  In order: ./worker_process source_dir target_dir filename operation
 *
 *  The program writes in stdout (fss_console redirects it to a pipe)
 *  The output of the program has the form:
 *      EXEC_REPORT_START
 *      SOURCE DIR: <source directory>
 *      TARGET DIR: <target directory>
 *      OPERATION: <operation>
 *      STATUS: {SUCCESS,PARTIAL,ERROR}
 *      DETAILS: {number of files copied,skipped,deleted}
 *      ERRORS: // only if the program has errors this is printed 
 *          - file <name>: <error that happened>
 *          .
 *          .
 *      EXEC_REPORT_END
* */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include "../include/worker_process.h"
#define MAX_BYTES_READ 1024
#define PERMS 0666 // Permitions for creating files



int main(int argc,char* argv[]) {
    if (argc != 5) {
        // This error happened because of wrong usage by parent process, so we 
        //  can't continuue the program
        write(1,"ERROR in <worker_process>! Wrong number of arguments\n",29);
        exit(-1);
    }
    // We wan't to print all our output after we end the procedure as a message,
    //  so we save all errors we can find in that buffer, and print it after
    char error_buffer[1024];
    // We start with an empty string so we can use strcat
    error_buffer[0] = '\0'; 
    int files_copied = 0;
    int files_skipped = 0;
    int files_deleted = 0;
    int error_code;
    bool error_flag = false;
    // We want to synchronize every single file in the directory
    if (!strcmp(argv[4],"FULL")) {
        DIR* source_ptr = opendir(argv[1]);
        DIR* target_ptr = opendir(argv[2]);
        if (source_ptr == NULL || target_ptr == NULL) {
            // Write error message
            error_flag = true;
            strcat(error_buffer,strerror(errno));
            strcat(error_buffer,"\n");
        }
        else {
            // We will first delete every file in target,hat way remaining 
            // files in target are removed
            struct dirent* direntp;
            while ((direntp = readdir(target_ptr)) != NULL) {
                // We skip . and .. directories 
                if (strcmp(direntp->d_name,".") == 0 || strcmp(direntp->d_name,"..") == 0)
                    continue;
                if ((delete_file(argv[2],direntp->d_name)) < 0) {
                    error_flag = true;
                    files_skipped++;
                    strcat(error_buffer,"- File ");
                    strcat(error_buffer,direntp->d_name);
                    strcat(error_buffer,": ");
                    strcat(error_buffer,strerror(errno));
                    strcat(error_buffer,"\n");
                }
                else 
                    files_deleted++;
            }
            // Now we will copy everything from source to target (except . and ..)
            while ((direntp = readdir(source_ptr)) != NULL) {
                if (strcmp(direntp->d_name,".") == 0 || strcmp(direntp->d_name,"..") == 0)
                    continue;
                if ((synchronize_files(argv[1],argv[2],direntp->d_name)) < 0) {
                    files_skipped++;
                    error_flag = true;
                    strcat(error_buffer,"- File ");
                    strcat(error_buffer,direntp->d_name);
                    strcat(error_buffer,": ");
                    strcat(error_buffer,strerror(errno));
                    strcat(error_buffer,"\n");
                }
                else {
                    files_copied++;
                }
            }
        }    
    }
    // We want to create a new file and copy content's from source to it 
    else if (!strcmp(argv[4],"ADDED")) {
         char* filepath = malloc(sizeof(char) * (strlen(argv[1]) + 
        // File path is source_dir/filename\0
        strlen(argv[3]) + 2)); 
        if (filepath == NULL) {
            // This is an error that happened through memory corruption or 
            // overflow, so we can't continue
            write(2,strerror(errno),strlen(strerror(errno)));
            exit(-1);
        }
        // Make filepath 
        strcpy(filepath,argv[1]);
        strcat(filepath,"/");
        strcat(filepath,argv[3]);
        if ((open(filepath,O_CREAT | O_WRONLY | O_TRUNC) < 0))  {
            // So we can print the error in the following conditional
            error_code = -1;
       }
        else {
            error_code = synchronize_files(argv[1],argv[2],argv[3]);
        }        
        // Check if an error happened
        if (error_code == -1) {
            error_flag = true;
            files_skipped++;
            strcat(error_buffer,"- File ");
            strcat(error_buffer,argv[3]);
            strcat(error_buffer,": ");
            strcat(error_buffer,strerror(errno));
            strcat(error_buffer,"\n");
        }           
        else 
            files_copied++;
   
    }        
    // We open the target file for writing and we copy evertything to it
    else if (!strcmp(argv[4],"MODIFIED")) {
        error_code = synchronize_files(argv[1],argv[2],argv[3]);
        if (error_code == -1) {
            error_flag = true;
            files_skipped++;
            strcat(error_buffer,"- File ");
            strcat(error_buffer,argv[3]);
            strcat(error_buffer,": ");
            strcat(error_buffer,strerror(errno));
            strcat(error_buffer,"\n");
        }
        else 
            files_copied++;
    }
    // We delete the target file 
    else if (!strcmp(argv[4],"DELETED")) {
        error_code = delete_file(argv[2], argv[3]);
        if (error_code == -1) {
            error_flag = true;
            files_skipped++;
            strcat(error_buffer,"- File ");
            strcat(error_buffer,argv[3]);
            strcat(error_buffer,": ");
            strcat(error_buffer,strerror(errno));
            strcat(error_buffer,"\n");
        }
        else 
            files_deleted++;       

    }
    else {
        fprintf(stderr,"ERROR! Wrong operation value: <%s>\n",argv[4]);
        exit(-1);
    }
    // Writing output to stdout 
    write(1,"EXEC_REPORT_START\n",18);
    write(1,"SOURCE DIR: ",12);
    write(1,argv[1],strlen(argv[1]));
    write(1,"\nTARGET DIR: ",13);
    write(1,argv[2],strlen(argv[2]));
    write(1,"\nOPERATION: ",12);
    write(1,argv[4],strlen(argv[4]));
    write(1,"\nSTATUS: ",9);
    if (error_flag == false) {
        write(1,"SUCCESS\n",8);
    }
    else if (files_deleted == 0 && files_copied == 0) {
        write(1,"ERROR\n",6);
    }
    else {
        write(1,"PARTIAL\n",8);
    }
    write(1,"DETAILS: ",9);
    char num_buffer[32];
    if (files_copied > 0) {
        write(1,number_to_string(num_buffer,files_copied),strlen(num_buffer));
        write(1," files copied,",14);
    }
    if (files_deleted > 0) {
        write(1,number_to_string(num_buffer,files_deleted),strlen(num_buffer));
        write(1," files deleted,",15);
    }
    if (files_skipped > 0) {
        number_to_string(num_buffer,files_skipped);
        write(1,num_buffer,strlen(num_buffer));
        write(1," files skipped",14);
    }
    // We had errors to print
    if (error_flag == true) {
        write(1,"\nERRORS:",8);
        write(1,error_buffer,strlen(error_buffer));
    }
    write(1,"\nEXEC_REPORT_END\n",17);
    // We sent the SIGCHLD signal to the parent
    kill(getppid(),SIGCHLD);
    return 0; 
}

int delete_file(char* target_dir,char* filename) {
    char* filepath_target = malloc(sizeof(char) * (strlen(target_dir) + 
    strlen(filename) + 2)); // File path is target_dir/filename\0
    if (filepath_target == NULL) {
        write(2,strerror(errno),strlen(strerror(errno)));
        exit(-1);
    }
    // Make filepath using strcpy
    strcpy(filepath_target,target_dir);
    strcat(filepath_target,"/");
    strcat(filepath_target,filename);
    return unlink(filepath_target);
}

int synchronize_files(char* source_dir,char* target_dir,char* filename) {
    char* filepath_source = malloc(sizeof(char) * (strlen(source_dir) + 
    strlen(filename) + 2)); // File path is source_dir/filename\0
    if (filepath_source == NULL) {
        write(2,strerror(errno),strlen(strerror(errno)));
        exit(-1);
    }
    // Make filepath using strcpy and strcat
    strcpy(filepath_source,source_dir);
    strcat(filepath_source,"/");
    strcat(filepath_source,filename);

    // Do the same for target file
    char* filepath_target = malloc(sizeof(char) * (strlen(target_dir) + 
                strlen(filename) + 2)); // File path is target_dir/filename\0
    if (filepath_target == NULL) {
        return -1;
    }
    // Make filepath
    strcpy(filepath_target,target_dir);
    strcat(filepath_target,"/");
    strcat(filepath_target,filename);
    
    //We open both of those files one for reading,other for writing
    int fd_source = open(filepath_source,O_RDONLY);

    int fd_target = open(filepath_target,O_CREAT| O_WRONLY | O_TRUNC,PERMS);
    if (fd_source < 0 || fd_target < 0) {
        return -1;
    }

    // Now we copy everything from source to target
    int n;
    char buff[MAX_BYTES_READ];
    while ((n = read(fd_source,buff,MAX_BYTES_READ)) > 0) {
        if ((write(fd_target,buff,n)) < n) {
            close(fd_source);
            close(fd_target);
            return -1;
        }
    }
    close(fd_source);
    close(fd_target);
    if (n < 0) {
        return -1;
    }
    // Operation completed succesfully
    return 0;
}


char* number_to_string(char* buffer,int n) {
    int i = 0;
    if (n == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return buffer;
    }
    while (n > 0) {
        buffer[i++] = n % 10 + '0';
        n /= 10;
    }
    buffer[i] = '\0';
    i--;
    for (int j = 0; j < i; j++) {
        int temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;
        i--;
    }
    return buffer;
}


