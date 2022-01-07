/*  Copyright 2017-2022 Unpublished Work of Technologic Systems, Inc. dba embeddedTS
*  All Rights Reserved.
*
*  THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL,
*  PROPRIETARY AND TRADE SECRET INFORMATION OF embeddedTS.
*  ACCESS TO THIS WORK IS RESTRICTED TO (I) embeddedTS EMPLOYEES
*  WHO HAVE A NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE OF THEIR
*  ASSIGNMENTS  AND (II) ENTITIES OTHER THAN embeddedTS WHO
*  HAVE ENTERED INTO  APPROPRIATE LICENSE AGREEMENTS.  NO PART OF THIS
*  WORK MAY BE USED, PRACTICED, PERFORMED, COPIED, DISTRIBUTED, REVISED,
*  MODIFIED, TRANSLATED, ABRIDGED, CONDENSED, EXPANDED, COLLECTED,
*  COMPILED,LINKED,RECAST, TRANSFORMED, ADAPTED IN ANY FORM OR BY ANY
*  MEANS,MANUAL, MECHANICAL, CHEMICAL, ELECTRICAL, ELECTRONIC, OPTICAL,
*  BIOLOGICAL, OR OTHERWISE WITHOUT THE PRIOR WRITTEN PERMISSION AND
*  CONSENT OF embeddedTS . ANY USE OR EXPLOITATION OF THIS WORK
*  WITHOUT THE PRIOR WRITTEN CONSENT OF embeddedTS  COULD
*  SUBJECT THE PERPETRATOR TO CRIMINAL AND CIVIL LIABILITY.
*/
/* To compile tsmicroctl, use the appropriate cross compiler and run the
* command:
*
*  arm-linux-gnueabihf-gcc -fno-tree-cselim -Wall -O0 -DCTL -o tsmicroctl tsmicroctl.c
*
*/

#include <stdio.h>
#include <unistd.h>
#include <dirent.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef CTL
#include <getopt.h>
#endif

#include "i2c-dev.h"

const char copyright[] = "Copyright (c) embeddedTS - " __DATE__ ;

#ifdef CTL
int model = 0;

int get_model()
{
	FILE *proc;
	char mdl[256];
	char *ptr;

	proc = fopen("/proc/device-tree/model", "r");
	if (!proc) {
	    perror("model");
	    return 0;
	}
	fread(mdl, 256, 1, proc);
	ptr = strstr(mdl, "TS-");
	return strtoull(ptr+3, NULL, 16);
}
#endif

int micro_init()
{
	static int fd = -1;
	fd = open("/dev/i2c-0", O_RDWR);
	if(fd != -1) {
		if (ioctl(fd, I2C_SLAVE_FORCE, 0x78) < 0) {
			perror("Microcontroller did not ACK 0x78\n");
			return -1;
		}
	}

	return fd;
}

#ifdef CTL

// Scale voltage to silabs 0-2.5V
uint16_t inline sscale(uint16_t data){
	return data * (2.5/1023) * 1000;
}

// Scale voltage for resistor dividers
uint16_t inline rscale(uint16_t data, uint16_t r1, uint16_t r2)
{
	uint16_t ret = (data * (r1 + r2)/r2);
	return sscale(ret);
}

void do_info(int twifd)
{
	uint8_t data[17];
	bzero(data, 17);
	read(twifd, data, 17);

	printf("bootmode=0x%x\n", (data[15] >> 3) & 0x1);
	printf("revision=0x%x\n", (data[16] >> 4) & 0xf);

	printf("reboot_source=");

	switch(data[15] & 0x7) {
	  case 0:
		printf("poweron\n");
		break;
	  case 1:
		printf("WDT\n");
		break;
	  case 2:
		printf("resetswitch\n");
		break;
	  case 3:
		printf("sleep\n");
		break;
	  case 4:
		printf("uart_break\n");
		break;
	  default:
		printf("unknown\n");
		break;
	}
}

static void usage(char **argv) {
	fprintf(stderr, "Usage: %s [OPTION] ...\n"
	  "embeddedTS Microcontroller Access\n"
	  "\n"
	  "  -i, --info              Get info about the microcontroller\n"
	  "  -X, --resetswitchon     Enable reset switch\n"
	  "  -Y, --resetswitchoff    Disable reset switch\n"
	  "  -M, --timewkup=<time>   Time in seconds to wake up after\n"
	  "  -m, --resetswitchwkup   Wake up at reset switch press\n"
	  "  -L, --sleep             Sleep CPU, must specify -M|-m to wake\n"
	  "                          Note that this shuts off power and may\n"
	  "                          be unsafe without proper precautions\n"
	  "  -h, --help              This message\n",
	  argv[0]
	);
}

int main(int argc, char **argv)
{
	int c;
	int twifd;

	int opt_info = 0;

	int opt_timewkup = 0xffffff, opt_sleepmode = 0, opt_resetswitchwkup = 0;
	char dat[4];

	int opt_resetswitchon = 0, opt_resetswitchoff = 0;

	static struct option long_options[] = {
	  { "info", 0, 0, 'i' },
	  { "resetswitchon", 0, 0, 'X' },
	  { "resetswitchoff", 0, 0, 'Y' },
	  { "timewkup", 1, 0, 'M' },
	  { "resetswitchwkup", 0, 0, 'm' },
	  { "sleep", 0, 0, 'L' },
	  { "help", 0, 0, 'h' },
	  { 0, 0, 0, 0 }
	};

	if(argc == 1) {
		usage(argv);
		return(1);
	}

	model = get_model();
	switch(model) {
	  case 0x7400:
		break;
	  default:
		fprintf(stderr, "Unsupported model\n");
		return 1;
	}

	twifd = micro_init();
	if(twifd == -1)
	  return 1;

	

	while((c = getopt_long(argc, argv, 
	  "iXYM:mLh",
	  long_options, NULL)) != -1) {
		switch (c) {
		  case 'i':
			opt_info = 1;
			break;
		  case 'X':
			opt_resetswitchon = 1;
			break;
		  case 'Y':
			opt_resetswitchoff = 1;
			break;
		  case 'L':
			opt_sleepmode = 1;
			break;
		  case 'M':
			opt_timewkup = strtoul(optarg, NULL, 0);
			break;
		  case 'm':
			opt_resetswitchwkup = 1;
			break;
		  case 'h':
		  default:
			usage(argv);
			return 1;
		}
	}

	if (opt_info) {
		do_info(twifd);
	}

        if(opt_resetswitchon) {
                dat[0] = 0x42;
                write(twifd, &dat[0], 1);
        } else if(opt_resetswitchoff) {
                dat[0] = 0x40;
                write(twifd, &dat[0], 1);
        }

	if(opt_sleepmode) {
		dat[0]=(0x1 | (opt_resetswitchwkup << 1) |
		  ((opt_sleepmode-1) << 4) | 1 << 6);
		dat[3] = (opt_timewkup & 0xff);
		dat[2] = ((opt_timewkup >> 8) & 0xff);
		dat[1] = ((opt_timewkup >> 16) & 0xff);
		write(twifd, &dat, 4);
	}

	return 0;
}

#endif
