#! /bin/bash 
cd /tmp || exit
DISPORDER="m k u"
for DISP in $DISPORDER; do
    FILE_NAMES=$(ls -1 "$DISP"-*)
    for FILE in $FILE_NAMES; do
        FILE_CONTENT=$(cat "$FILE")
        echo "$FILE","$FILE_CONTENT"
    done
    echo
done

KERNEL_START=$(cat /tmp/m-kernelstart-time)
KERNEL_MAIN=$(cat /tmp/k-start-time)
echo "Kernel Decompression time",$(( KERNEL_MAIN - KERNEL_START))

USER_ENTRY=$(cat /tmp/k-user-space-entry-time)
echo "Kernel Exec time",$(( USER_ENTRY - KERNEL_START))
