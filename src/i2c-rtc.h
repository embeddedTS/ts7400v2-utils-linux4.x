#ifndef __I2C_RTC_H_
#define __I2C_RTC_H_

int i2c_rtc_init();
int i2c_rtc_read(int twifd, uint8_t chip, char *data, char addr, int size);
int i2c_rtc_write(int twifd, uint8_t chip, char *data, char addr, int size);

#endif
