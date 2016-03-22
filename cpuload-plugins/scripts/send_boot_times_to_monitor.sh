
readproc
echo "TABLE: MLO_ENTRY $(cat /tmp/m-entry-time) msec" > /tmp/socfifo
usleep 100000
echo "TABLE: MLO_EXIT $(cat /tmp/m-kernelstart-time) msec" > /tmp/socfifo
usleep 100000
echo "TABLE: KERNEL_ENTRY $(cat /tmp/k-start-time) msec" > /tmp/socfifo
usleep 100000
echo "TABLE: USERSPACE_ENTRY $(cat /tmp/k-user-space-entry-time) msec" > /tmp/socfifo
usleep 100000
