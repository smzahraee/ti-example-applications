/*
 * Copyright (C) 2015 Texas Instruments
 * Author: Karthik Ramanan <karthik.ramanan@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include "statcoll.h"

#define PAGE_SIZE 4096

#define EMIF1_BASE 0x4c000000
#define EMIF2_BASE 0x4d000000

#define EMIF_PERF_CNT_1     0x80
#define EMIF_PERF_CNT_2     0x84
#define EMIF_PERF_CNT_CFG   0x88
#define EMIF_PERF_CNT_TIM   0x90

static unsigned
tv_diff(struct timeval *tv1, struct timeval *tv2)
{
    return (tv2->tv_sec - tv1->tv_sec) * 1000000 +
        (tv2->tv_usec - tv1->tv_usec);
}


struct emif_perf {
    int code;
    const char *name;
};

static const struct emif_perf emif_perf_tab[] = {
    {  0, "access"     },
    {  1, "activate"   },
    {  2, "read"       },
    {  3, "write"      },
    {  4, "fifo_cmd"   },
    {  5, "fifo_write" },
    {  6, "fifo_read"  },
    {  7, "fifo_ret"   },
    {  8, "prio"       },
    {  9, "cmd_pend"   },
    { 10, "data"       },
};

static void *emif1, *emif2;
static int BANDWIDTH=0;
static int DELAY = 1;
static int EMIF_PERF_CFG1 = 9;
static int EMIF_PERF_CFG2 = 10;


static int STATCOLL=0;
static int TOTAL_TIME;
static int INTERVAL_US;

struct timeval t1, t2;

FILE* outfile;
struct emif_stats {
    uint32_t cycles;
    uint32_t cnt1;
    uint32_t cnt2;
};

static struct emif_stats emif1_start, emif1_end;
static struct emif_stats emif2_start, emif2_end;

static void *emif_init(int fd, unsigned base)
{
    void *mem =
        mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, base);
    volatile uint32_t *emif = mem,temp;
    
   if (mem == MAP_FAILED){
        return NULL;
    }

    emif[EMIF_PERF_CNT_CFG>>2] = EMIF_PERF_CFG2 << 16 | EMIF_PERF_CFG1;
   
    return mem;
}

static void emif_read(volatile uint32_t *emif, struct emif_stats *st)
{
    st->cycles = emif[EMIF_PERF_CNT_TIM>>2];
    st->cnt1   = emif[EMIF_PERF_CNT_1>>2];
    st->cnt2   = emif[EMIF_PERF_CNT_2>>2];
}

static void emif_print(const char *tag, struct emif_stats *st1,
                       struct emif_stats *st2)
{
    uint32_t cycles = st2->cycles - st1->cycles;
    uint32_t cnt1   = st2->cnt1   - st1->cnt1;
    uint32_t cnt2   = st2->cnt2   - st1->cnt2;
    printf("%s %s %2llu%% %s %2llu%%", tag,
           emif_perf_tab[EMIF_PERF_CFG1].name, 100ull*cnt1/cycles,
           emif_perf_tab[EMIF_PERF_CFG2].name, 100ull*cnt2/cycles);
    fprintf(outfile,"%s%s= %2llu,%s%s= %2llu,", 
           tag, emif_perf_tab[EMIF_PERF_CFG1].name, 100ull*cnt1/cycles,
           tag, emif_perf_tab[EMIF_PERF_CFG2].name, 100ull*cnt2/cycles);
}

static int perf_init(void)
{
    int fd = open("/dev/mem", O_RDWR);
    int err = 0;

    if (fd == -1){
       printf("error fd=open() \n");     
       return -1;
    }
    emif1 = emif_init(fd, EMIF1_BASE);
    emif2 = emif_init(fd, EMIF2_BASE);

    if (!emif1 || !emif2){
         printf("error if (!emif1 || !emif2) \n");       
         err = -1;
    }

    close(fd);
    return err;
}

static void perf_start(void)
{
    if (emif1) {
        emif_read(emif1, &emif1_start);
        emif_read(emif2, &emif2_start);
    }
}

static void perf_stop(void)
{
    if (emif1) {
        emif_read(emif1, &emif1_end);
        emif_read(emif2, &emif2_end);
    }
}

static void perf_print(void)
{
    if (emif1) {
        emif_print("EMIF1", &emif1_start, &emif1_end);
        printf("\t");
        emif_print("EMIF2", &emif2_start, &emif2_end);
        printf("\r");
	fprintf(outfile, "\n");
	fflush(outfile);
	fflush(stdout);
    }
}

static void perf_close(void)
{
    if (emif1) munmap(emif1, PAGE_SIZE);
    if (emif2) munmap(emif2, PAGE_SIZE);
}

static int get_cfg(const char *name, int def)
{
    char *end;
    int n = strtol(name, &end, 0);
    int i;

    if (!*end)
        return n;

    for (i = 0; i < sizeof(emif_perf_tab)/sizeof(emif_perf_tab[0]); i++)
        if (!strcmp(name, emif_perf_tab[i].name))
            return emif_perf_tab[i].code;

    return def;
}


unsigned int emif_freq()
{
    volatile unsigned *tim1;
    unsigned v1, v2;
    int fd;
    
    /*calculation EMIF frequency 
      EMIF_PERF_CNT_TIM = \n32-bit counter that 
      continuously counts number for 
      EMIF_FCLK clock cycles elapsed 
      after EMIFis brought out of reset*/

    fd = open("/dev/mem", O_RDONLY);
    if (fd == -1) {
        perror("/dev/mem");
        return 1;
    }

    void *mem =
    mem = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_SHARED, fd, EMIF1_BASE);
    if (mem == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    tim1 = (unsigned *)((char*)mem + EMIF_PERF_CNT_TIM);

    v1 = *tim1;
    gettimeofday(&t1, NULL);
    sleep(2);
    v2 = *tim1;
    gettimeofday(&t2, NULL);
    
    munmap(mem, PAGE_SIZE);
    close(fd);

    return (v2 - v1) / tv_diff(&t1, &t2);

}


char config_file_path[100];
char keylist[][50] = {
	"DELAY",
	"EMIF_PERF_CFG1",
	"EMIF_PERF_CFG2",
	"BANDWIDTH",
	"STATCOLL",
	"TOTAL_TIME",
	"INTERVAL_US",
	"INITIATORS",
};

char line[512], *p;
char tokens[6][512];
int temp, flag = 0;
char *keyvalue, *pair;
char key[100];
int linecount=0;


int debug=0;

void print_valid_options(void)
{
   int i;
        printf("Invalid key found\n");
	printf("Supported keys are :\n");
	for(i=0; i<sizeof(keylist)/sizeof(keylist[0]); i++)
		printf("\t\t %s\n", keylist[i]);

}
int validatekey(char *ptr)
{
	int i;
	for(i=0; i<sizeof(keylist)/sizeof(keylist[0]); i++)
		if(strcmp(ptr, keylist[i]) == 0)
			return 0;

	return 1;
}

void add_key_value(char *key, int value)
{
	printd("%s", "Inside add_key_value\n");
	
	if(strcmp(key, "BANDWIDTH") == 0) {
		BANDWIDTH = value;
		return;
	}
	if(strcmp(key, "STATCOLL") == 0) {
		STATCOLL = value;
		return;
	}
	else
		printd("%s", "********** UNKNOWN**********");

	if(BANDWIDTH == 1) {
		if(strcmp(key, "DELAY") == 0)
			DELAY = value;
		else if(strcmp(key, "EMIF_PERF_CFG1") == 0)
			EMIF_PERF_CFG1 = value;
		else if(strcmp(key, "EMIF_PERF_CFG2") == 0)
			EMIF_PERF_CFG2 = value;
	}
	else
		printf("NOTE: BANDWIDTH is not enabled, ignoring %s\n", key);


        if(STATCOLL == 1) {
		if(strcmp(key, "INTERVAL_US") == 0)
			INTERVAL_US = value;
		else if(strcmp(key, "TOTAL_TIME") == 0)
			TOTAL_TIME = value;
        }
	else
		printf("NOTE: STATCOLL is not enabled, ignoring %s\n", key);
}

void bandwidth_usage() {

    printf("#########################################################\n##\n"

           "##  usage    : ./Dra7xx_ddrstat   <DELAY>  <EMIF_PERF_CFG1>  <EMIF_PERF_CFG2> \n"
           "##  default  :                    DELAY=1  EMIF_PERF_CFG1=9  EMIF_PERF_CFG2=10\n"	
           "##  option   : for EMIF_PERF_CFG1 and EMIF_PERF_CFG2\n"
           "##             0  -> access,\n"  
           "##             1  -> activate,\n"
           "##             2  -> read,\n"
           "##             3  -> write,\n"
           "##             4  -> fifo_cmd,\n"
           "##             5  -> fifo_write,\n"
           "##             6  -> fifo_read,\n"
           "##             7  -> fifo_ret,\n"
           "##             8  -> prio,\n"
           "##             9  -> cmd_pend,\n"
           "##             10 -> data    \n##\n"

           "##  EMIF frq : %d MHz\n\n", emif_freq() );
}


void print_usage()
{
     printf("USAGE: glsdkstatcoll -f config.ini\n"
             "\n There should be another file called initiators.cfg that should be present in the same directory\n"
             "\n LIST OF INITIATORS \n"
             "\n STATCOL_EMIF1_SYS"
             "\n STATCOL_EMIF2_SYS"
             "\n STATCOL_MA_MPU_P1"
             "\n STATCOL_MA_MPU_P2"
             "\n STATCOL_MPU1"
             "\n STATCOL_MMU1"
             "\n STATCOL_TPTC_RD1"
             "\n STATCOL_TPTC_WR1"
             "\n STATCOL_TPTC_RD2"
             "\n STATCOL_TPTC_WR2"
             "\n STATCOL_VIP1_P1"
             "\n STATCOL_VIP1_P2"
             "\n STATCOL_VIP2_P1"
             "\n STATCOL_VIP2_P2"
             "\n STATCOL_VIP3_P1"
             "\n STATCOL_VIP3_P2"
             "\n STATCOL_VPE_P1"
             "\n STATCOL_VPE_P2"
             "\n STATCOL_EVE1_TC0"
             "\n STATCOL_EVE1_TC1"
             "\n STATCOL_EVE2_TC0"
             "\n STATCOL_EVE2_TC1"
             "\n STATCOL_EVE3_TC0"
             "\n STATCOL_EVE3_TC1"
             "\n STATCOL_EVE4_TC0"
             "\n STATCOL_EVE4_TC1"
             "\n STATCOL_DSP1_MDMA"
             "\n STATCOL_DSP1_EDMA"
             "\n STATCOL_DSP2_MDMA"
             "\n STATCOL_DSP2_EDMA"
             "\n STATCOL_IVA"
             "\n STATCOL_GPU_P1"
             "\n STATCOL_GPU_P2"
             "\n STATCOL_BB2D_P1"
             "\n STATCOL_DSS"
             "\n STATCOL_CSI2_2"
             "\n STATCOL_MMU2"
             "\n STATCOL_IPU1"
             "\n STATCOL_IPU2"
             "\n STATCOL_DMA_SYSTEM_RD"
             "\n STATCOL_DMA_SYSTEM_WR"
             "\n STATCOL_CSI2_1"
             "\n STATCOL_USB3_SS"
             "\n STATCOL_USB2_SS"
             "\n STATCOL_USB2_ULPI_SS1"
             "\n STATCOL_USB2_ULPI_SS2"
             "\n STATCOL_PCIE_SS1"
             "\n STATCOL_PCIE_SS2"
             "\n STATCOL_DSP1_CFG"
             "\n STATCOL_DSP2_CFG"
             "\n STATCOL_GMAC_SW"
             "\n STATCOL_PRUSS1_P1"
             "\n STATCOL_PRUSS1_P2"
             "\n STATCOL_PRUSS2_P1"
             "\n STATCOL_PRUSS2_P2"
             "\n STATCOL_DMA_CRYPTO_RD"
             "\n STATCOL_DMA_CRYPTO_WR"
             "\n STATCOL_MPU2"
             "\n STATCOL_MMC1"
             "\n STATCOL_MMC2"
             "\n STATCOL_SATA"
             "\n STATCOL_MLBSS"
             "\n STATCOL_BB2D_P2"
             "\n STATCOL_IEEE1500"
             "\n STATCOL_DBG"
             "\n STATCOL_VCP1"
             "\n STATCOL_OCMC_RAM1"
             "\n STATCOL_OCMC_RAM2"
             "\n STATCOL_OCMC_RAM3"
             "\n STATCOL_GPMC"
             "\n STATCOL_MCASP1"
             "\n STATCOL_MCASP2"
             "\n STATCOL_MCASP3"
             "\n STATCOL_VCP2"
             "\n STATCOL_MA \n\n");

}

int main(int argc, char **argv)
{
    int option;
    FILE *fp;
    int i;
    

    
    /* Read config file */
    /* Initialize this to turn off verbosity of getopt */
    opterr = 0;

    while ((option = getopt (argc, argv, "hdf:")) != -1)
    {
	    switch(option)
	    {
		    case 'f':
			    strcpy(config_file_path, optarg);
			    break;
		    case 'd':
			    debug=1;
			    break;
		    case 'h':
			    print_usage();
                            exit(0);
		    default:
			    printf("Invalid option.. Exiting\n");
			    exit(0);
	    }
    }

    fp = fopen(config_file_path, "r");
    if (fp == NULL) {
	    fprintf(stderr, "couldn't open the specified config file.. \n");
	    fprintf(stderr, "USAGE: ./glsdkstatcoll -f config.ini\n");
	    fprintf(stderr, "\n\n For help:  ./glsdkstatcoll -h \n");
	    return -1;
    }

    while (fgets(line, sizeof line, fp)) {
	    printd("Line is = %s", line);

	    if (line[0] == '#' || line[0] == '\n') {
		    continue;
	    }

	    memset(tokens, 0, sizeof(tokens));
	    i = 0;

	    pair = strtok (line," ,");
	    while (pair != NULL)
	    {
		    printd ("\tPair is = %s\n",pair);
		    strcpy(tokens[i++], pair);
		    pair = strtok (NULL, " ,.-");
	    }

	    for(temp=0; temp< i; temp++)
	    {
		    printd("Line %d: %s\n", temp, tokens[temp]);

		    keyvalue = strtok (tokens[temp]," =");
		    while (keyvalue != NULL)
		    {
			    if(flag == 0)
			    {
				    if(validatekey(keyvalue))
				    {
                                            print_valid_options();
					    exit(0);
				    }
				    strcpy(key, keyvalue);
				    printd ("\tKey is = %s\n",key);
				    flag++;
			    }
			    else
			    {
				    printd ("\tValue is = %s",keyvalue);
				    printd (" (%d)\n", atoi(keyvalue));
				    add_key_value(key, atoi(keyvalue));
				    flag = 0;
			    }
			    keyvalue = strtok (NULL, " =");
		    }
	    }



	    linecount++;
	    printd("%s", "------------------- \n");

    }

    fclose(fp);

    printf("\n\nCOMPLETED: Parsing of the user specified parameters.. \n \
                \nConfiguring device now.. \n\n");
    if(BANDWIDTH == 1) {
	    bandwidth_usage();
	    if (DELAY <= 0)
		    DELAY = 1;

	    if (perf_init()){
		    printf("perf_init return non zero \n");
		    return 1;
	    }

	    outfile = fopen("emif-performance.csv", "w+");
	    if (!outfile) {
		    printf("\n Error opening file");
	    }
	    for (;;) {
		    perf_start();
		    sleep(DELAY);
		    perf_stop();
		    perf_print();
	    }

	    fclose(outfile);
	    perf_close();
	    return 0;
    }

    if(STATCOLL == 1) {
	    printf("STATISTICS COLLECTOR option chosen\n");
            printf("------------------------------------------------\n\n");
	    fp = fopen("initiators.cfg", "r");
	    if (fp == NULL) {
		    fprintf(stderr, "couldn't open the specified file initiators.cfg'\n");
		    return -1;
	    }

	    int i=0;
            char list[100][50];
	    memset(list, sizeof(list), 0);
	    while (fgets(line, sizeof line, fp)) {
	 	    printd("Line is = %s", line);
		    /* Slightly strange way to chop off the \n character */
		    strtok(line, "\n");
		    strcpy(list[i++], line);
	    }
	    fclose(fp);

	    statcoll_start(TOTAL_TIME, INTERVAL_US, list);
    }

}

