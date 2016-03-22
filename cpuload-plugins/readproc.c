/*
 * Copyright (c) 2015 Venkateswara Rao Mandela (venkat.mandela@ti.com)
 *
 * Read boot time data from device tree and dump it to text files
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <endian.h>
#include <math.h>

int32_t boot_vals[32];


char *boot_paths[] = {
	"m-entry-time",
	"m-boardinit-time",
	"m-heap-init-dur",
	"m-spi-init-dur",
	"m-mmc-init-dur",
	"m-image-load-dur",
	"m-kernelstart-time",

	"m-display-time",
	"m-dsp1start-time",
	"m-dsp2start-time",
	"m-ipu1start-time",
	"m-ipu2start-time",

	"k-start-time",
	"k-hwmod-dur",
	"k-mm-init-dur",
	"k-rest-init-time",
	"k-cust-machine-dur",
	"k-user-space-entry-time",
	"theend"
};

int32_t get_u32(char *filename)
{
	FILE *fp;
	int32_t val=0;

	fp = fopen(filename,"rb");
	if(fp==NULL) {
		printf("Unable to open file\n");
		return 0;
	}
	fread(&val,sizeof(int32_t),1,fp);
	val = be32toh(val);
	fclose(fp);
	return val;
}

void store_boot_times()
{
	char path[128];
	char *path_prefix = "/proc/device-tree/chosen";
	char out_path[128];
	int i = 0;

	for(i=0;i<32;i++) {
		int32_t val;
		FILE *fp;
		if(strcmp(boot_paths[i],"theend")==0)
			return;
		sprintf(path,"%s/%s",path_prefix,boot_paths[i]);
		val = get_u32(path);
		boot_vals[i]=val;
		sprintf(out_path,"/tmp/%s",boot_paths[i]);
		fp=fopen(out_path,"wt");
		fprintf(fp,"%d\n",(int)floor((1.0f*val)/32.768f));
		fclose(fp);
	}

	return;
}

int main(int argc, char **argv)
{

	store_boot_times();

    return 0;

}
