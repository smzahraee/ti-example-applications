/*
  Copyright (c) 2015 Karthik Ramanan (a0393906@ti.com)
  Sample code for using 32K SYNC counter from user space
*/

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "read32k.h"

#define SYNC_COUNTER_32K_BASE 0x4AE04000
#define COUNTER_32K 0x30
#define PAGE_SIZE 4096

static void *sync_counter;

int32_t counter_init(void)
{

    int err = 0;

    int fd = open("/dev/mem", O_RDWR);

    if (fd == -1){
        printf("error fd=open() \n");
        return -1;
    }

    sync_counter =  mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, SYNC_COUNTER_32K_BASE);

    if (!sync_counter){
        printf("ERROR: mmap failed\n");
        err = -1;
    }

    close(fd);

    return err;

}

uint32_t counter_read()
{
    return ((volatile uint32_t *)sync_counter)[COUNTER_32K >> 2];

}

uint32_t counter_read_ms()
{
	uint32_t capt_time = counter_read();
	return (uint32_t)(floor((1.0f*(float)capt_time)/32.768f));

}
