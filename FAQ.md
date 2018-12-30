

Do you have the exact dimensions, or even better a CAD drawing of the sensor in 3D?

Sensors are 18mm x 149mm. 3D model is available here: 



 how to set up multiple sensors connected to one Arduino Uno. 

 What is the expected life of the 'Modbus RS485 soil moisture sensor' version rugged, por continuous use outdoor?

 I want to know if I can use a 2 meter long cable without affecting the sensor measurements

 How to solder wire to the sensor?

 Light sensor of rugged variant?

 Do you offer the source code from the i2c chrip project?



Communication with ESP8266 is unreliable, sensor sends data then stops working.

https://github.com/Apollon77/I2CSoilMoistureSensor/issues/8

If this does not help, slow down the bus speed, that will get the communication to work reliably.

Let me know how it goes. 


Can you provide a formal EU invoice.

Durability of non-rugged sensors.

standard solder mask seems to last about 3 months before flaking off (thats without epoxy)

Can you set different address for each sensor before shipping? 

## Rugged sensors

**How do I connect the sensor to my board?**

Wire colors go like this:
  RED - VCC
  BLACK - GND
  BLUE or GREEN- SDA
  YELLOW - SCK 

You can leave the metal sheet and metal wire unconnected.

You need to add pull up resistors to both SCL and SDA.



 I would like to have sensors with a cable up to 2 meter 

 # RS485

 If I wanted to use 8 of these sensors for an application I would only need 1 usb dongle ?
