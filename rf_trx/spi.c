/*
 * SPI testing utility (using spidev driver)
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 */

#undef BCM

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#ifdef BCM
#include <bcm2835.h>
#define SPI_CLOCK_DIVIDER 26
#endif

static void pabort(const char *s)
{
	perror(s);
	abort();
}

static const char *device = "/dev/spidev0.1";
static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 10000000;
static uint16_t delay;
static int fd;

void dump (char *ttl, uint8_t *buf, int len) {

	puts(ttl);
        int ret;
	for (ret = 0; ret < len; ret++) {
		if (!(ret % 6))
			puts("");
		printf("%.2X ", buf[ret]);
	}
	puts("");
}

int spi_init() {
	int ret = 0;

	mode = 0;
	delay = 0;

#ifdef BCM
	if (!bcm2835_init())
		return 0;
	bcm2835_spi_begin();	
	bcm2835_spi_setClockDivider(SPI_CLOCK_DIVIDER); 			// 250MHz / 26 = 9.6MHz
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE0); 				// CPOL = 0, CPHA = 0
	bcm2835_spi_chipSelect(BCM2835_SPI_CS1);					// chip select 1
	return 1;
#else
	fd = open(device, O_RDWR);
	if (fd < 0)
		pabort("can't open device");

	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");

	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);

	return 1;
#endif
}

void spi_transfern(char *buf, uint32_t len) {
	int ret;
        //dump("TR tx", (uint8_t *)buf, len);
	
#ifdef BCM
	bcm2835_spi_transfern(buf, len);
#else
	uint8_t *rx = malloc(len);
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)buf,
		.rx_buf = (unsigned long)rx,
		.len = len,
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1) {
		free(rx);
		pabort("can't send spi message");
	}
	memcpy(buf, rx, len);
	free(rx);
#endif
}

void spi_writenb(char * buf, uint32_t len) {
	int ret;
	dump("WR tx", (uint8_t *)buf, len);
#ifdef BCM
	bcm2835_spi_writenb(buf, len);
#else
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)buf,
		.rx_buf = (unsigned long)0,
		.len = len,
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");
#endif
}
