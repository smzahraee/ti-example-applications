sh dummy_boot_time_results.sh 
sh getVoltage.sh 
while true 
do
sh getFrequency.sh
sh dummy_cpu_load.sh 
sh getTemp.sh 
sleep 1
done
