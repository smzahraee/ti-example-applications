#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "PVRScopeStats.h"

extern "C" {

// Step 1. Define a function to initialise PVRScopeStats 
unsigned int PSInit(struct SPVRScopeImplData **ppsPVRScopeData, struct SPVRScopeCounterDef **ppsCounters, struct SPVRScopeCounterReading* const psReading, unsigned int* const pnCount) 
{ 
	//Initialise PVRScope 
	const enum EPVRScopeInitCode eInitCode = PVRScopeInitialise(ppsPVRScopeData); 
	if(ePVRScopeInitCodeOk == eInitCode) 
	{ 
		printf("Initialised services connection.\n"); 
	} 
	else 
	{ 
		printf("Error: failed to initialise services connection.\n"); 
		*ppsPVRScopeData = NULL; 
		return 0; 
	} 
	//Initialise the counter data structures. 
	if (PVRScopeGetCounters(*ppsPVRScopeData, pnCount, ppsCounters, psReading)) 
	{ 
		printf("Total counters enabled: %d.", *pnCount); 
	} 
	return 1; 
}

void usage(void)
{
	printf("ERROR: Invalid usage\n\n");
	printf("\t pvrscope <option> <time_seconds>\n");
	printf("\n");
	printf("\t option: \n");
	printf("\t        -f write to fifo (/tmp/socfifo)\n");
	printf("\t        -c write to console \n");
	printf("\t time_seconds = 0 to run infinitely\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	unsigned int usleep_duration = 50000;
	unsigned int sampleRate = 1000000/usleep_duration; 
	unsigned int index = 0; 
	unsigned int i = 0;
	unsigned int choice;
	unsigned int console = 1;
	int fd=0;
	char *gpufifo = (char *)"/tmp/socfifo";


	//Internal control data 
	struct SPVRScopeImplData *psData; 
	//Counter information (set at uint time) 
	struct SPVRScopeCounterDef *psCounters; 
	unsigned int uCounterNum; 

	//Counter reading data 
	unsigned int uActiveGroupSelect; 
	unsigned int bActiveGroupChanged; 
	struct SPVRScopeCounterReading sReading;

        if(argc < 3) {
		usage();
	}

	while ((choice = getopt(argc, argv, "fc")) != -1) {

		switch (choice) {
		case 'f':
			console = 0;
			break;
		case 'c':
			console = 1;
			break;
		default:
			usage();
			break;
		}
	}

	unsigned int NUMBER_OF_LOOPS = atoi(argv[2]);
	printf("NUMBER_OF_LOOPS = %d\n", NUMBER_OF_LOOPS);

	if (PSInit(&psData, &psCounters, &sReading, &uCounterNum))  
	{
		printf("PVRScope up and running. \n"); 
	}
	else 
	{
		printf("Error initializing PVRScope.\n");
		exit(0);
	}

	for(int i = 0; i < uCounterNum; ++i) 
	{ 
		printf(" Group %d %s %d\n", psCounters[i].nGroup, psCounters[i].pszName, psCounters[i].nBoolPercentage); 
	}


	// Step 3. Set the active group to 0 
	bActiveGroupChanged = true; 
	uActiveGroupSelect = -1; 

	if(!console)
		fd = open(gpufifo, O_WRONLY);

	unsigned int counter = 0;
	while (NUMBER_OF_LOOPS == 0 || counter < NUMBER_OF_LOOPS) 
	{ 
		// Ask for the active group 0 only on the first run. 
		if(bActiveGroupChanged) 
		{ 
			PVRScopeSetGroup(psData, uActiveGroupSelect); 
			bActiveGroupChanged = false; 
		} 
		++index; 
		if (index < sampleRate) 
		{ 
			// Sample the counters every 10ms. Don't read or output it. 
			if(PVRScopeReadCounters(psData, NULL) == 0) {
				/* Do nothing */
				//printf("Unable to get counters for index = %d\n", index);	
			}
		}
		else 
		{ 
			++counter; 
			index = 0; 
			// Step 4. Read and output the counter information for group 0 to Logcat 
			//printf("Reading counters \n");
			//if(PVRScopeReadCounters(psData, &sReading)) 
			
			if(PVRScopeReadCounters(psData, &sReading) != 0)
			{ 
				char sendbuffer[100];

				int gpuload = (int)((sReading.pfValueBuf[5]*sReading.pfValueBuf[4]/532000000.0 + sReading.pfValueBuf[6]*sReading.pfValueBuf[4]/532000000.0)*100)/100/2;
				if(gpuload > 100)
					gpuload = 100;
				sprintf(sendbuffer, "CPULOAD: GPU %d", gpuload);
				if(!console)
					write(fd, sendbuffer, strlen(sendbuffer));
				else
					printf("%s\n", sendbuffer);
			} 
			else {
				write(fd, "CPULOAD: GPU 0", strlen("CPULOAD: GPU 0"));
			}
		}

		//Poll for the counters once in 50 msecond 
		usleep(usleep_duration); 
	} 
	// Step 5. Shutdown PVRScopeStats 
	PVRScopeDeInitialise(&psData, &psCounters, &sReading);

	if(!console)
		close(fd);
	return 0;
}
}
