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

void util_get_cpu_usage(double *cpu_usage)
{
	static FILE *fp = NULL;
	char buf[256];
	uint64_t tot;
	uint64_t u, n, s, i, w, x, y, z;
	static uint64_t last_i = 0, last_total = 0;


	if (!fp) {
		if (!(fp = fopen("/proc/stat", "r")))
			fprintf(stderr, "Failed /proc/stat open: %s", strerror(errno));
	}
	if (fp) {
		while (1) {
			rewind(fp);
			fflush(fp);
			if (!fgets(buf, sizeof(buf), fp)) {
				fprintf(stderr, "failed /proc/stat read\n");
			} else {
				sscanf(buf, "cpu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
						&u,
						&n,
						&s,
						&i,
						&w,
						&x,
						&y,
						&z
					  );
				if (last_total == 0) {
					last_total = u+n+s+i+w+x+y+z;
					last_i = i;
					usleep(100000);
				} else {
					tot = u+n+s+i+w+x+y+z;
					*cpu_usage = (1.0 - ((double)(i-last_i)/(double)(tot-last_total)));
					last_i = i;
					last_total = tot;
					break;
				}
			}
		}
	}
}

int main(int argc, char *argv[])
{
	int fd=0;

	int index = 0;
	unsigned int NUMBER_OF_LOOPS = atoi(argv[2]);
	//char *fifo = (char *)"/tmp/nonlinuxfifo";
	char *fifo = argv[1];
	char sendbuffer[100];

	printf("The fifo is %s\n", fifo);
	printf("Total seconds = %d\n", NUMBER_OF_LOOPS);

	fd = open(fifo, O_WRONLY);
	printf("FD = %d\n", fd);

	while(index < NUMBER_OF_LOOPS)
	{
		double cpu_load;
		util_get_cpu_usage(&cpu_load);
		printf("\n cpuload = %d\n", (int)(cpu_load*100));
		sprintf(sendbuffer, "CPULOAD: MPU %d", (int)(cpu_load*100)); 
		write(fd, sendbuffer, strlen(sendbuffer));

		sleep(1); 
		index++;
	}

	close(fd);
	return 0;
}
