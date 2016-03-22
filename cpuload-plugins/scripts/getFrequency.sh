LIST=(
IPU1
IPU2
DSP1
DSP2
)

omapconf show opp 2> /dev/null | grep -v omapconf | grep MHz  > frequency.txt

i=0
while [ $i -lt ${#LIST[@]} ];
do
        item=${LIST[$i]}
	echo $item
	
	cat frequency.txt | grep $item 
	value=`cat frequency.txt | grep $item | awk -F" " '{ print $6 }' | sed s/\(//`
	echo Frequency is $value

        echo "TABLE: FREQ_$item $value MHz" > /tmp/socfifo
        usleep 100000

        let "i = $i + 1"
done

cat frequency.txt | grep MPU 
value=`cat frequency.txt | grep MPU | awk -F" " '{ print $8 }'`
echo "TABLE: FREQ_MPU $value MHz" > /tmp/socfifo
usleep 100000

value=`cat frequency.txt | grep GPU | awk -F" " '{ print $6 }'`
echo "TABLE: FREQ_GPU $value MHz" > /tmp/socfifo
usleep 100000

cat frequency.txt | grep IVA 
value=`cat frequency.txt | grep IVA | awk -F" " '{ print $6 }'`
echo "TABLE: FREQ_IVA $value MHz" > /tmp/socfifo
usleep 100000

rm frequency.txt
