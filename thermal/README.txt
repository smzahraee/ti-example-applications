GLSDK Thermal Information Utility helper document v1.0

About:

The thermal management driver exports a lot of information collected from sensors,
emulation framework and cooling devices to sysfs filesystem.
Thermal utility is a shell script that helps user in reading this information
in a easy to understand way. It also make sit easy to emulate temperature on any core and/or load
cpu using a load simulator.

The thermal utility helps in
	-	reading information exported to the sysfs filesystem by
		the thermal management driver,
	-	interacting with the emulation interface
	-	heating cpu to see if the temperature changes are sensed and reflected

Installation/building:
A. Reading information and/or interact with emulation interface
	1. copy the thermal.sh script to target filesystem and change permissions to executable
		chmod +X thermal.sh
B. Heating cpu
	2. Build the cpuloadgen utility for target(cross compile), copy the binary to target filesystem
		a. The cpuloadgen utility source code should have been downloaded as part of GLSDK setup
		if you cant find it then download it from below repo
		https://github.com/ptitiano/cpuloadgen.git

Usage:
	1. Execute the script on the console, it will display current temperature of all thermal zones
		thermal.sh
	2. Chose what you want to do next from the menu

NOTE: You need to build the kernel with CONFIG_THERMAL_EMULATION enabled to use the emulation interface.



