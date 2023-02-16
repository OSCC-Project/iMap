#!/bin/bash
# bash run_refactor_ifpga_list.sh exe_path/refactor 1 list.txt 60 >>logfile 2>&1

files=`cat $3`
for element in ${files[@]}
do  
    echo $element
    if echo "$element" | grep -q -E '\.aig$'
    then
        timeout $4 $1 $2 $element;
    else
        echo "$element not end with .txt"
    fi
done