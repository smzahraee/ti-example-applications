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
#include <dirent.h>

int32_t get_u32(char *filename)
{
	FILE *fp;
	int32_t val=0;

	fp = fopen(filename,"rb");
	if(fp==NULL) {
		printf("Unable to open file %20s\n",filename);
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
	DIR *d;
	struct dirent *dir;
	d = opendir(path_prefix);

	if(!d)
		return;

	while ((dir = readdir(d)) != NULL) {
		int32_t val;
		FILE *fp;
		char *name = dir->d_name;
		if( ((strncmp(name,"k-",2)==0) ||
		     strncmp(name,"m-",2)==0)) {
			sprintf(path,"%s/%s",path_prefix,name);
			val = get_u32(path);
			sprintf(out_path,"/tmp/%s",name);
			fp=fopen(out_path,"wt");
			fprintf(fp,"%d\n",(int)floor((1.0f*val)/32.768f));
			fclose(fp);
		}
	}

	closedir(d);
	return;
}

int main(int argc, char **argv)
{

	store_boot_times();

	return 0;

}
