LIST=(
VDD_MPU
VDD_CORE
VDD_IVA
VDD_GPU
VDD_DSPEVE
)


i=0
while [ $i -lt ${#LIST[@]} ];
do
        item=${LIST[$i]}
	echo $item
	
	value=`omapconf show opp 2> /dev/null | grep -v omapconf | grep -w $item | awk -F" " '{ print $10 " " $11 }'`
	echo Voltage is $value

        echo "TABLE: $item $value" > /tmp/socfifo
        usleep 100000

        let "i = $i + 1"
done

