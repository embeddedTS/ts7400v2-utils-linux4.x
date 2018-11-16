#include <stdio.h>
#include <unistd.h>
#include <dirent.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "i2c-dev.h"
#include "i2c-rtc.h"

int i2c_rtc_init()
{
	static int fd = -1;

	if(fd != -1)
		return fd;

	fd = open("/dev/i2c-0", O_RDWR);

	return fd;
}

int i2c_rtc_read(int twifd, uint8_t chip, char *data, char addr, int bytes)
{
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg msgs[2];

	msgs[0].addr	= chip;
	msgs[0].flags	= 0;
	msgs[0].len	= 1;
	msgs[0].buf	= &addr;

	msgs[1].addr	= chip;
	msgs[1].flags	= I2C_M_RD;
	msgs[1].len	= bytes;
	msgs[1].buf	= data;

	packets.msgs = msgs;
	packets.nmsgs = 2;

	if(ioctl(twifd, I2C_RDWR, &packets) < 0) {
		perror("Unable to send data");
 		return 1;
    }
    return 0;
}

int i2c_rtc_write(int twifd, uint8_t chip, char *data, char addr, int size)
{
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg msg;
	uint8_t outdata[129];

	/* Linux only supports 4k transactions at a time, and we need
	 * two bytes for the address */
	assert(size <= 128);

	outdata[0] = addr;
	memcpy(&outdata[1], data, size);

	msg.addr	= chip;
	msg.flags	= 0;
	msg.len		= 1 + size;
	msg.buf		= (char *)outdata;

	packets.msgs  = &msg;
	packets.nmsgs = 1;

	if(ioctl(twifd, I2C_RDWR, &packets) < 0) {
		return 1;
	}
	return 0;
}
