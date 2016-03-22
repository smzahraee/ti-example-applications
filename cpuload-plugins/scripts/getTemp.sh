function covertTemp {
    temperature=0.0
    temp=0
    device_temp=$1
    diff=$(echo $device_temp-833 | bc)
    if [ $(echo " $diff >= 0" | bc) -eq 1 ]; then
        temperature=$(echo 0.40470*$diff | bc)
        temperature=$(echo $temperature+80.6 | bc)
    else 
        diff=$(echo $device_temp-735.721 | bc)
        if [ $(echo " $diff >= 0" | bc) -eq 1 ]; then
            temperature=$(echo 0.41196*$diff | bc)
            temperature=$(echo $temperature+40.6 | bc)
        else
            diff=$(echo $device_temp-639.875 | bc)
            if [ $(echo " $diff >= 0" | bc) -eq 1 ]; then
                temperature=$(echo 0.41820*$diff | bc)
                temperature=$(echo $temperature+0.6 | bc)
            else
                diff=$(echo $device_temp-544.177 | bc)
                temperature=$(echo 0.42240*$diff | bc)
                temperature=$(echo $temperature-39.8 | bc)
            fi
        fi
    fi
    echo $temperature
}

##############################################################################
#### Temperature Readings
##############################################################################
# MPU Temperature
regval=`omapconf --force omap5430 read 0x4A00232C`
regval=`echo "ibase=16; $regval" | bc`
let "regval = $regval & 1023"
MPUTemp=$(covertTemp $regval)

# GPU Temperature
regval=`omapconf --force omap5430 read 0x4A002330`
regval=`echo "ibase=16; $regval" | bc`
let "regval = $regval & 1023"
GPUTemp=$(covertTemp $regval)

# Core Temperature
regval=`omapconf --force omap5430 read 0x4A002334`
regval=`echo "ibase=16; $regval" | bc`
let "regval = $regval & 1023"
CoreTemp=$(covertTemp $regval)

# DSPEVE Temperature
regval=`omapconf --force omap5430 read 0x4A002574`
regval=`echo "ibase=16; $regval" | bc`
let "regval = $regval & 1023"
DspEveTemp=$(covertTemp $regval)

# IVA Temperature
regval=`omapconf --force omap5430 read 0x4A002578`
regval=`echo "ibase=16; $regval" | bc`
let "regval = $regval & 1023"
IvaTemp=$(covertTemp $regval)


echo "TABLE: MPU  $MPUTemp C" > /tmp/socfifo
usleep 100000
echo "TABLE: CORE $CoreTemp C" > /tmp/socfifo
usleep 100000
echo "TABLE: IVA $IvaTemp C" > /tmp/socfifo
usleep 100000
echo "TABLE: GPU $GPUTemp C" > /tmp/socfifo
usleep 100000
echo "TABLE: DSPEVE $DspEveTemp C" > /tmp/socfifo
usleep 100000

