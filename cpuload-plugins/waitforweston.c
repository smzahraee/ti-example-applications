#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include "read32k.h"


int main() {
	struct wl_display *disp;
	counter_init();
	/*Loop till connect to weston*/
	while((disp = wl_display_connect(NULL)) == NULL){
		usleep(10);
	}

	FILE *fp = fopen("/tmp/weston-start","wt");
	fprintf(fp,"%d\n",counter_read_ms());
	close(fp);
	return 0;
}
