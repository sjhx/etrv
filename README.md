# etrv
Modifications to energenie eTRV downloads 

As downloaded the energenie code did not work for me on a Pi2 even after updating to the latest bcm2835 library which is required for Pi2.
I wasnt really happy with the direct access to device registers through that library so started to change the code to drive spi through the spi device driver (enable spi in raspi-config).
My second unhappiness with the energenie code were multiple files with the same name but slightly different content so my other change is to make a common library. (ToDo)

At this point the listener works but eTRV_Menu hangs (I should try the original eTRV_Menu on a Pi1 next).
