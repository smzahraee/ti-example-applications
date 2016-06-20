#include <sys/types.h>
#include <libdce.h>
#include <omap_drm.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
        if(argc != 4) {
                printf("ERROR: Usage is \
                        \n rprocinfo fifoname num_seconds remote_proc_name\
			\n    remote_proc_name for IPU2=ivahd_vidsvr\
			\n    remote_proc_name for DSP1=dsp_vidsvr\
                        \n\n Example: rprocinfo /tmp/socfifo 100 ivahd_vidsvr");
                return 0;
        }

        int fd=0;
        int index = 0;
        int NUMBER_OF_LOOPS = atoi(argv[2]);
        char *fifo = argv[1];
	char *rproc_name=argv[3];
        char sendbuffer[100];

        printf("The fifo is %s\n", fifo);
        printf("Total seconds = %d\n", NUMBER_OF_LOOPS);

        fd = open(fifo, O_WRONLY);

        Engine_Handle engine;
        Engine_Error ec;
        struct omap_device *dev = NULL;

        dev = dce_init();
        if(dev == NULL) {
                printf("dce init failed\n");
        }

        engine = Engine_open(argv[3], NULL, &ec);
        if (!engine) {
                printf("could not open engine\n");
                exit(0);
        }


        while(NUMBER_OF_LOOPS==-1 || index < NUMBER_OF_LOOPS) {
                int output = get_rproc_info(engine, RPROC_CPU_LOAD);
                memset(sendbuffer, 0x0, 100);
  		if(strcmp(argv[3], "ivahd_vidsvr") == 0) {
                	sprintf(sendbuffer, "CPULOAD: IPU2 %d", (int)(output));
		}
		else {
                	sprintf(sendbuffer, "CPULOAD: DSP1 %d", (int)(output));
		}
			
                write(fd, sendbuffer, strlen(sendbuffer));

                sleep(1);
                index++;
        }

        Engine_close(engine);
        dce_deinit(dev);
        close(fd);
}

