/*  Copyright 2011-2022 Unpublished Work of Technologic Systems, Inc. dba embeddedTS
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

#include <stdio.h>
#include <unistd.h>
#include <dirent.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "gpiolib.h"
#include "i2c-rtc.h"

const char copyright[] = "Copyright (c) embeddedTS - " __DATE__ ;

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

static void usage(char **argv) {
	fprintf(stderr, "Usage: %s [OPTION] ...\n"
	  "embeddedTS Hardware access\n"
	  "\n"
	  "  -i, --info              Print device information\n"
	  "  -o, --rtcinfo           Print RTC power on/off timestamp\n"
	  "  -v, --nvram             Get/Set RTC NVRAM\n"
	  "  -V, --cpuadc            Read and print CPU LRADC values\n"
	  "  -h, --help              This message\n",
	  argv[0]
	);
}

int main(int argc, char **argv)
{
	int i;

	int opt_info = 0;
	
	int opt_rtcinfo = 0;
	char rtcdat[11];

	int opt_nvram = 0;
	int rtcfd = 0;
	uint32_t nvdat[32], x;
	char *cdat = (char *)nvdat;
	char *e, var[8];

	int opt_cpuadc = 0;
	FILE *lradcfd;
	char path[64];
	int32_t val;

	static struct option long_options[] = {
	  { "info", 0, 0, 'i' },
	  { "rtcinfo", 0, 0, 'o' },
	  { "nvram", 0, 0, 'v' },
	  { "cpuadc", 0, 0, 'V' },
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

	while((i = getopt_long(argc, argv, 
	  "iovVh",
	  long_options, NULL)) != -1) {
		switch (i) {
		  case 'i':
			opt_info = 1;
			break;
		  case 'o':
			opt_rtcinfo = 1;
			break;
		  case 'v':
			opt_nvram = 1;
			break;
		  case 'V':
			opt_cpuadc = 1;
			break;
		  case 'h':
		  default:
			usage(argv);
			return 1;
		}
	}

	if (opt_info) {
		printf("model=%X\n", get_model());
	}

	if (opt_nvram) {
		rtcfd = i2c_rtc_init();
		i2c_rtc_read(rtcfd, 0x57, cdat, 0x0, 128);
		for (i = 0; i < 32; i++) {
			sprintf(var, "nvram%d", i);
			e = getenv(var);
			if (e) {
				x = strtoul(e, NULL, 0);
				i2c_rtc_write(rtcfd, 0x57, (char *)&x, i<<2, 4);
			}
		}
		for (i = 0; i < 32; i++) printf("nvram%d=0x%x\n", i, nvdat[i]);
        }

        if (opt_rtcinfo) {
		rtcfd = i2c_rtc_init();

		i2c_rtc_read(rtcfd, 0x6F, &rtcdat[1], 0x16, 5);
		i2c_rtc_read(rtcfd, 0x6F, &rtcdat[6], 0x1b, 5);

		printf("rtcinfo_firstpoweroff=%02x%02x%02x%02x%02x\n",
		  rtcdat[5], rtcdat[4], (rtcdat[3] & 0x7f), rtcdat[2],
		  rtcdat[1]);
		printf("rtcinfo_lastpoweron=%02x%02x%02x%02x%02x\n",
		  rtcdat[10], rtcdat[9], (rtcdat[8] & 0x7f), rtcdat[7],
		  rtcdat[6]);
		/* Read the current setting of brownout voltages, set by kernel
		 * Then clear the timestamp reg */
		i2c_rtc_read(rtcfd, 0x6F, &rtcdat[0], 0x9, 1);
		rtcdat[0] |= 0x80;
		i2c_rtc_write(rtcfd, 0x6F, &rtcdat[0], 0x9, 1);
        }

	if (opt_cpuadc) {
		/* NOTE: Kernel driver handles most of this, we just attach and
		 * provide scaling.
		 *
		 * The math can be greatly simplified using just a single
		 * multiply and shift so the end mV value fits in a 16bit var.
		 *
		 * Channel 0 is connected to diode for temp sensing.
		 */
		for (i = 1; i <= 3; i++) {
			snprintf(path, 64,
			  "/sys/bus/iio/devices/iio:device0/in_voltage%d_raw",
			  i);
			lradcfd = fopen(path, "r");
			fscanf(lradcfd, "%d", &val);
			printf("LRADC_ADC%d_millivolts=%d\n", i,
			  ((val * 28807) >> 15));
			fclose(lradcfd);
		}
	}

	return 0;
}
