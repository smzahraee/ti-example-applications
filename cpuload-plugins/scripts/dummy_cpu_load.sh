delay=100000
	echo "CPULOAD: MPU $((RANDOM%100))" > /tmp/socfifo
	usleep $delay
	echo "CPULOAD: IPU1 $((RANDOM%100))" > /tmp/socfifo
	usleep $delay
	echo "CPULOAD: IPU2 $((RANDOM%100))" > /tmp/socfifo
	usleep $delay
	echo "CPULOAD: DSP1 $((RANDOM%100))" > /tmp/socfifo
	usleep $delay
	echo "CPULOAD: DSP2 $((RANDOM%100))" > /tmp/socfifo
	usleep $delay
	echo "CPULOAD: GPU $((RANDOM%100))" > /tmp/socfifo
	usleep $delay
