delay=100000
echo "CPULOAD: MPU $((RANDOM%100))" > /tmp/socfifo
usleep $delay
echo "CPULOAD: GPU $((RANDOM%100))" > /tmp/socfifo
usleep $delay
