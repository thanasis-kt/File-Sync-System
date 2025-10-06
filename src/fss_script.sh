#!/bin/bash

# This script will be used to either delete (purge command) a logfile/target directory,
#  to list some source directories that have a given property (still monitored, or stopped)
#  or to list all directories 
 
#We find if a file stopped with 
#cat aa | grep Monitoring | cut -d"]" -f2 | grep /home/user/docs | tail -n 1 | awk '{print $2}'

filename="" # example we will take from args later
command=""

# Transform arguments to an array
args=("$@")

# Iterate throught the arguments to find our parameters

for ((i = 0; i <= ${#args[@]} - 1; i++)); do
    if [ "${args[i]}" == "-p" ]; then
        path=${args[i + 1]}
    elif [  "${args[i]}" == "-c" ]; then
        command=${args[i + 1]}
    fi
done

if [ -z "$path" ] || [ -z "$command" ]; then
     echo "ERROR! Wrong type of arguments given" 
     exit -1
fi

if [ -d "${path}" ] && ["$command" != "purge"]; then
    echo "ERROR! Can't use this command with a directory"
fi

# The state that we search through in the file
target_state=""

if [ "$command" == "listAll" ]; then
    target_state="*" # We will use this as wildcard
elif [ "$command" == "listMonitored" ]; then
    # We list all the files that say started
    target_state="started"
elif [ "$command" == "listStopped" ]; then
    target_state="stopped"
elif [ "$command" == "purge" ]; then
    # We delete all the files recursively
    echo "Deleting $path"
    rm -r $path
    # rm sucess code is 0
    if [ $? -eq 0 ]; then
        echo "Purge complete."
        exit 0
    else
        echo "Purge failed"
        exit -1
    fi

else
    echo "ERROR! Wrong command given"
    exit -1
fi

## We filter the files by making them unique (using sort -u option). We only keep the filename
# The files that we are interested have at least one entry:  
#   "[TIMESTAMP] Monitoring started/stopped for <filename>
# We sort them with -u to remove duplicates
files_to_check=(`cat $path | grep "Monitoring" | cut -d"]" -f2 | awk '{print $4}'  | sort  -u`)

for file in "${files_to_check[@]}"; do
    state=`cat $path | grep Monitoring | cut -d"]" -f2 | grep $file | tail -n 1 | awk '{print $2}'`
    
    # This checks if a file state is what we looking for (matching states or
    #  wildcard)
    if [ "$state" == "$target_state" ] || [ "$target_state" == "*" ]; then
        line=(`cat $path | grep "$file" | grep "FULL" | tail -n 1 | tr "[]" "  "`)
        echo "${line[2]} -> ${line[3]} [Last Sync: ${line[0]} ${line[1]}] [${line[6]}]"
    fi
done

exit 0
