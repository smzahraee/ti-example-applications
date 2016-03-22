LIST=(
MLO_ENTRY
SPLASH_SCREEN
AUDIO_START
KERNEL_START
WESTON_START
HMI_START
2D_SRV_START
3D_SRV_START
)


i=0
while [ $i -lt ${#LIST[@]} ];
do
	item=${LIST[$i]}
	
	echo "TABLE: $item  $((RANDOM%10000)) msec" > /tmp/socfifo
	usleep 100000

	let "i = $i + 1"
done
