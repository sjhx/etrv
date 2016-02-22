#include "app_main.h"
#include "dev_HRF.h"
#include "decoder.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "spi.h"
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

// receive in variable length packet mode, display and resend. Data with swapped first 2 bytes
int main(int argc, char **argv){
	uint32_t sensorId = 0x102031;
	uint8_t manufacturerId = 0x04;
	uint8_t productId = 0x3;
	uint8_t encryptId = 0xf2;
	uint32_t data  = 0x1020;
	char message_type = 0;
    		
	int option;
	int quiet = 0;
	uint8_t queued_data = FALSE;
extern	uint8_t recieve_temp_report;
	
	while ((option = getopt(argc, argv, "e:m:p:s:d:q")) != -1)
	{
        
		switch(option)
		{
			case 'e':
				encryptId = strtoul(optarg, NULL, 0);
				
				break;
			case 'm':
				manufacturerId = strtoul(optarg, NULL, 0);
				
				break;
			case 'p':
				productId = strtoul(optarg, NULL, 0);
				
				break;
			case 's':
				sensorId = strtoul(optarg, NULL, 0);
				
				break;
			case 'd':
				data = strtoul(optarg, NULL, 0);
				 queued_data = TRUE;
				break;	
			case 'q':
				quiet = 1;
				break;
			default:
				printf("Syntax: %s [options]\n\n"
					   "\t-e\tEncryption ID to use\n"
					   "\t-m\tManufacturer ID to send\n"
					   "\t-p\tProduct ID to send\n"
					   "\t-s\tSensor ID to send\n"
					   "\t-d\tdata of TEMP SET to send\n"					   				   
					   "\t-q\tQuiet (send no comands)\n", argv[0]);
				return 2;
		}
	}	
	
	//ask what message is to be sent   
	printf("Message to send \n"
			"t - set room temp 30 degC \n"
			"u - update room temperature 36degC \n"
			"? - indentify \n"
			"e - exercise valve \n"
			"b - request eTRv battery voltage \n"
			"d - request diagnostic flags \n"
			"x - set valve state open \n"
			"y - set valve state closed \n"
			"z - set valve stste normal \n"
			"l - set low power mode on \n"
			"n - set low power mode off \n"
			"a - acknowledge only\n"
			"r - set reporting interval to 15 minutes \n"
			"m - monitor only - Non acknowledgement\n"
			">");
	message_type = getchar();
    
	
	
	if (!quiet)
		printf("Sending to %02x:%02x:%06x, encryption %02x\n",
			   manufacturerId, productId, sensorId, encryptId);
			   
     if (queued_data) 
        {
        printf("queued data to be sent\n");			   
        } else {
        printf("Command %c will be sent on next temperature report\n",message_type);      
        }
        
        
#ifdef BCM
	if (!bcm2835_init())
		return 1;
#else
	if (!spi_init())
		return 1;
#endif

	time_t currentTime;
	time_t monitorControlTime;
	
#ifdef BCM
	// LED INIT
	bcm2835_gpio_fsel(LEDG, BCM2835_GPIO_FSEL_OUTP);			// LED green
	bcm2835_gpio_fsel(LEDR, BCM2835_GPIO_FSEL_OUTP);			// LED red
	bcm2835_gpio_write(LEDG, LOW);
	bcm2835_gpio_write(LEDR, LOW);
	// SPI INIT
	bcm2835_spi_begin();	
	bcm2835_spi_setClockDivider(SPI_CLOCK_DIIDER_26); 			// 250MHz / 26 = 9.6MHz
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE0); 				// CPOL = 0, CPHA = 0
	bcm2835_spi_chipSelect(BCM2835_SPI_CS1);					// chip select 1
#endif

	HRF_config_FSK();
	HRF_wait_for(ADDR_IRQFLAGS1, MASK_MODEREADY, TRUE);			// wait until ready after mode switching
	HRF_clr_fifo();

	monitorControlTime = time(NULL);
	while (1){
		currentTime = time(NULL);
		recieve_temp_report = FALSE;
				
		HRF_receive_FSK_msg(encryptId, productId, manufacturerId, sensorId );

		if (send_join_response)
		{
			if (message_type !='m')
			{
			printf("send JOIN response\n");
			HRF_send_FSK_msg(HRF_make_FSK_msg(manufacturerId, encryptId, productId, sensorId,
											  2, PARAM_JOIN_RESP, 0), encryptId);
			}
			send_join_response = FALSE;
		}
			
		if (recieve_temp_report)
		{
		#ifdef DEBUG
			printf("**** Received temp report \n");
		#endif
      
		   /*if (queued_data)
		    {
                                
			printf("send temp report\n");
			HRF_send_FSK_msg(HRF_make_FSK_msg(manufacturerId, encryptId, productId, sensorId,
											  4, PARAM_TEMP_SET, 0x92, (data & 0xff), (data >> 8 & 0xff)), encryptId);
			queued_data = FALSE;
	        recieve_temp_report = FALSE;
	         } */
	         switch (message_type)
			{
			case 't':
				printf("sending set temperature 30 degC \n");
				HRF_send_FSK_msg(HRF_make_FSK_msg(manufacturerId, encryptId, productId, sensorId,
											  4, 0xf4, 0x92, 0x00, 0xF0), encryptId);
				break;
			case 'u':
				printf("sending room temperature 36 degC \n");
				HRF_send_FSK_msg(HRF_make_FSK_msg(manufacturerId, encryptId, productId, sensorId,
											  4, 0x74, 0x92, 0x01, 0x20), encryptId);
				break;
			case '?':
				printf("sending identify \n");
				HRF_send_FSK_msg(HRF_make_FSK_msg(manufacturerId, encryptId, productId, sensorId,
											  2, 0xBF, 0), encryptId);
				break;
			case 'e':
			printf("sending exercise valve \n");
				HRF_send_FSK_msg(HRF_make_FSK_msg(manufacturerId, encryptId, productId, sensorId,
											  2, 0xA3, 0), encryptId);
				break;
			case 'b':
				printf("sending request battery volatge \n");
				HRF_send_FSK_msg(HRF_make_FSK_msg(manufacturerId, encryptId, productId, sensorId,
											  2, 0xE2, 0), encryptId);
				break;
			case 'd':
				printf("sending request diagnosics \n");
				HRF_send_FSK_msg(HRF_make_FSK_msg(manufacturerId, encryptId, productId, sensorId,
											  2, 0xA6, 0), encryptId);
				break;
			case 'l':
				printf("sending set low power mode on \n");
				HRF_send_FSK_msg(HRF_make_FSK_msg(manufacturerId, encryptId, productId, sensorId,
											  3, 0xA4, 0x01, 0x01), encryptId);
				break;
			case 'n':
				printf("sending set low power mode off \n");
				HRF_send_FSK_msg(HRF_make_FSK_msg(manufacturerId, encryptId, productId, sensorId,
											  3, 0xA4, 0x01, 0x00), encryptId);
			case 'x':
				printf("sending set valve open \n");
				HRF_send_FSK_msg(HRF_make_FSK_msg(manufacturerId, encryptId, productId, sensorId,
											  3, 0xA5, 0x01, 0x00), encryptId);
				break;
			case 'y':
				printf("sending set valve closed \n");
				HRF_send_FSK_msg(HRF_make_FSK_msg(manufacturerId, encryptId, productId, sensorId,
											  3, 0xA5, 0x01, 0x01), encryptId);
				break;
			case 'z':
				printf("sending set valve normal \n");
				HRF_send_FSK_msg(HRF_make_FSK_msg(manufacturerId, encryptId, productId, sensorId,
											  3, 0xA5, 0x01, 0x02), encryptId);
				break;
			case 'a':
				printf("sending NIL acknowledgement \n");
				HRF_send_FSK_msg(HRF_make_FSK_msg(manufacturerId, encryptId, productId, sensorId,
											  0), encryptId);
				break;
			case 'r':
				printf("sending set reporting onterval 15 minutes \n");
				HRF_send_FSK_msg(HRF_make_FSK_msg(manufacturerId, encryptId, productId, sensorId,
											  4, 0xD2, 0x02, 0x03, 0x84), encryptId);
			case 'm':
				//Listen only - do nothing
				break;
			default:
				printf("send NIL command\n");
				HRF_send_FSK_msg(HRF_make_FSK_msg(manufacturerId, encryptId, productId, sensorId,
											  0), encryptId);
			}
		}
  	    /*else {
            printf("send NIL command\n");
			HRF_send_FSK_msg(HRF_make_FSK_msg(manufacturerId, encryptId, productId, sensorId,
											  0), encryptId);
			recieve_temp_report = FALSE;  
      }*/
      
		
		
	/*	if (!quiet && difftime(currentTime, monitorControlTime) > 1)
		{
			monitorControlTime = time(NULL);
			static bool switchState = false;
			switchState = !switchState;
			printf("send temp message:\trelay %s\n", switchState ? "ON" : "OFF");
#ifdef BCM
			bcm2835_gpio_write(LEDG, switchState);
#endif
			HRF_send_FSK_msg(HRF_make_FSK_msg(manufacturerId, encryptId, productId, sensorId,
											  4, PARAM_TEMP_SET, 0x92, 0x10, 0x20),
							 encryptId);
		}
		*/
	}
#ifdef BCM
	bcm2835_spi_end();
#endif
	return 0;
}




