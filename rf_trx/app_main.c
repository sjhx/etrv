#include "app_main.h"
#include "dev_HRF.h"
#include <stdio.h>
#include <string.h>
#include "spi.h"
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

uint8_t g_seed_pid = 0x01;

// receive in variable length packet mode, display and send to R1 and Legacy socket.
int main(int argc, char **argv){
	if (!spi_init())
		return 1;
	if (argc > 1)
		g_seed_pid = atoi(argv[1]);
	
	time_t currentTime, legacyTime, monitorControlTime;
	
	// LED INIT
	//bcm2835_gpio_fsel(LEDG, BCM2835_GPIO_FSEL_OUTP);			// LED green
	//bcm2835_gpio_fsel(LEDR, BCM2835_GPIO_FSEL_OUTP);			// LED red
	//bcm2835_gpio_write(LEDG, LOW);
	//bcm2835_gpio_write(LEDR, LOW);
	// SPI INIT
	//bcm2835_spi_begin();	
	//bcm2835_spi_setClockDivider(SPI_CLOCK_DIVIDER_26); 			// 250MHz / 26 = 9.6MHz
	//bcm2835_spi_setDataMode(BCM2835_SPI_MODE0); 				// CPOL = 0, CPHA = 0
	//bcm2835_spi_chipSelect(BCM2835_SPI_CS1);					// chip select 1

	HRF_config_FSK();
	puts("FSK configured");
	HRF_wait_for(ADDR_IRQFLAGS1, MASK_MODEREADY, true);			// wait until ready after mode switching
	puts("Ready");
	HRF_clr_fifo();
	puts("FIFO clear");

	legacyTime = time(NULL);
	monitorControlTime = time(NULL);
	while (1){
		//currentTime = time(NULL);
		
		HRF_receive_FSK_msg();
		//puts("msg rcv");
		
		if (difftime(currentTime, legacyTime) >= 9)			// Number of seconds between Legacy message send
		{
			legacyTime = time(NULL);
			printf("send LEGACY message:\t");
			static bool switchState = false;
			switchState = !switchState;
			//bcm2835_gpio_write(LEDR, switchState);
			HRF_send_OOK_msg(switchState);
		}
		
		if (difftime(currentTime, monitorControlTime) >= 9)	// Number of seconds between R1 message send
		{
			monitorControlTime = time(NULL);
			static bool switchState = false;
			switchState = !switchState;
			printf("send MONITOR + CONTROL message:\trelay %s\n", switchState ? "ON" : "OFF");
			//bcm2835_gpio_write(LEDG, switchState);
			//HRF_send_FSK_msg(HRF_make_FSK_msg(0x02, 3, PARAM_SW_STATE | 0x80, 1, switchState));
			HRF_send_FSK_msg(HRF_make_FSK_msg(0xffffff, 3, PARAM_SW_STATE | 0x80, 1, switchState));
			//HRF_send_FSK_msg(HRF_make_FSK_msg(0xffffff, 2, PARAM_JOIN, 0));
		}
	}
	//bcm2835_spi_end();
	return 0;
}




