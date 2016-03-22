#! /bin/bash 
cd /tmp
FILE_NAMES=$(ls -1 [kmu]-*)
for FILE in $FILE_NAMES; do
    FILE_CONTENT=$(cat $FILE)
    echo $FILE,$FILE_CONTENT
done

