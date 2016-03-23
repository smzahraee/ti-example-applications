LIST=(
MLO_ENTRY
MLO_EXIT
KERNEL_ENTRY
USERSPACE_ENTRY
)


i=0
while [ $i -lt ${#LIST[@]} ];
do
	item=${LIST[$i]}
	
	echo "TABLE: $item  $((RANDOM%10000)) msec" > /tmp/socfifo
	usleep 100000

	let "i = $i + 1"
done
