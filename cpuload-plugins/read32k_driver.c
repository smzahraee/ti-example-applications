/*
  Copyright (c) 2015 Karthik Ramanan (a0393906@ti.com)
  Sample code for using 32K SYNC counter from user space
*/

#include <stdio.h>
#include <stdint.h>
#include "read32k.h"

int main(int argc, char **argv)

{

    unsigned int current, previous=0;

    if (counter_init()){

        printf("counter_init return non zero \n");

        return 1;

    }

    current = counter_read_ms();
    printf("%d\n",current);

    return 0;

}
