#!/bin/bash
if [[ "$#" -lt 4 ]]                                     #check if the number of parameters is right
    then
        echo 'wrong number of arguments'
    exit 1
fi
extra_arg=-1
args=( "$@" )                                           #create an array of the arguments
for (( i=0; i < $#; i++ ))
do
    if [[ "${args[$i]}" ==  "-l" ]]                     #stores the path
        then
            path=${args[$i+1]}
    elif [[ "${args[$i]}" ==  "-c" ]]                   #stores the command
        then
            command=${args[$i+1]}
            if [ "$command" ==  "size" ] && [ "${i+2}" -lt  "$#" ] && [ "${args[$i+2]}" !=  "-l" ]          #if the command is list and there is an extra argument
                then                                                                                        #which is not the -l flag then we save the number
                    extra_arg=${args[$i+2]}
            fi
    fi
done
if [[ "$command" ==  "size" ]]                          #we check which command we inserted and execute it
    then
        if [[ "$extra_arg" -gt 0 ]]                     #we check if there is an extra argument for size
            then
                du --max-depth=1 -h $path/*/ | sort -n -r | head -n $extra_arg
        else
                du --max-depth=1 -h $path/*/ | sort -n -r
        fi
elif [[ "$command" ==  "list" ]]
    then
        ls -1 $path
elif [[ "$command" ==  "purge" ]]
    then
        rm -R -- $path/*/
fi
