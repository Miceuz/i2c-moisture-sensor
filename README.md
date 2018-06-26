# i2c-moisture-sensor

I2C based soil moisture sensor. A continuation of the [Chirp - plant watering alarm](https://github.com/Miceuz/PlantWateringAlarm) project. There is also an [RS485](https://github.com/Miceuz/rs485-moist-sensor) and an [analog](https://github.com/Miceuz/soil-moisture-sensor-analog) version available.

## I2C protocol
Available registers for reading and writing.

  Name | Register | R/W | Data length 
-------|----------|-----|-------------
GET_CAPACITANCE   | 0x00 | (r) | 2
SET_ADDRESS       | 0x01 | (w) | 1
GET_ADDRESS       | 0x02 | (r) | 1
MEASURE_LIGHT     | 0x03 | (w) | 0
GET_LIGHT         | 0x04 | (r) | 2
GET_TEMPERATURE   | 0x05 | (r) | 2
RESET             | 0x06 | (w) | 0
GET_VERSION       | 0x07 | (r) | 1
SLEEP             | 0x08 | (w) | 0
GET_BUSY          | 0x09 | (r) | 1

GET_BUSY returns 1 if any measurement is in progress, 0 otherwise.

### Python library for Raspberry Pi

*NOTE: if you experience problems on Raspberry Pi 3, slow down I2C bus speed by adding this line to /boot/config.txt:*
**dtparam=i2c1_baudrate=30000**

[Göran Lundberg](https://github.com/ageir) has released a Python library for Raspberry Pi: [https://github.com/ageir/chirp-rpi](https://github.com/ageir/chirp-rpi)
It has a very comprehensive documentation and covers a lot of functionality.

Some features:
* Uses a trigger function to trigger all enabled sensors. User selectable.
* Get soil moisture in percent (requires calibration) or capacitance value.
* Several temperature scales to choose from. Celcius, Farenheit and Kelvin.
* Offset to calibrate the temperature sensor.
* Measurement timestamps for all on board sensors.
* Built in support for changing the I2C address of the sensor.
* Deep sleep mode to conserve power.
* Calibration tool for soil moisture.

### Raspberry Pi examples

This is interface class provided by Daniel Tamm and Jasper Wallace
```python
#!/usr/bin/python
# cannot use python3 because smbus not working there
# Modified script from https://github.com/JasperWallace/chirp-graphite/blob/master/chirp.py
# by DanielTamm

import smbus, time, sys

class Chirp:
	def __init__(self, bus=1, address=0x20):
		self.bus_num = bus
		self.bus = smbus.SMBus(bus)
		self.address = address
    
	def get_reg(self, reg):
		# read 2 bytes from register
		val = self.bus.read_word_data(self.address, reg)
		# return swapped bytes (they come in wrong order)
		return (val >> 8) + ((val & 0xFF) << 8)

	def reset(self):
		# To reset the sensor, write 6 to the device I2C address
		self.bus.write_byte(self.address, 6)

	def set_addr(self, new_addr):
		# To change the I2C address of the sensor, write a new address
		# (one byte [1..127]) to register 1; the new address will take effect after reset
		self.bus.write_byte_data(self.address, 1, new_addr)
		self.reset()
		self.address = new_addr

	def moist(self):
		# To read soil moisture, read 2 bytes from register 0
		return self.get_reg(0)

	def temp(self):
		# To read temperature, read 2 bytes from register 5
		return self.get_reg(5)

	def light(self):
		# To read light level, start measurement by writing 3 to the
		# device I2C address, wait for 3 seconds, read 2 bytes from register 4
		self.bus.write_byte(self.address, 3)
		time.sleep(1.5)
		return self.get_reg(4)

	def __repr__(self):
		return "<Chirp sensor on bus %d, addr %d>" % (self.bus_num, self.address)

if __name__ == "__main__":
	addr = 0x20
	if len(sys.argv) == 2:
		if sys.argv[1].startswith("0x"):
			addr = int(sys.argv[1], 16)
		else:
			addr = int(sys.argv[1])
	chirp = Chirp(1, addr)

	print chirp
	print "Moisture\tTemperature\tBrightness"
	while True:
		print "%d\t%d\t%d" % (chirp.moist(), chirp.temp(), chirp.light())
		time.sleep(1)
```

This is another RasPi example provided by user *krikk*
```python
#!/usr/bin/python

#https://github.com/adafruit/Adafruit-Raspberry-Pi-Python-Code/tree/master/Adafruit_I2C
from Adafruit_I2C import Adafruit_I2C
from time import sleep, strftime
from datetime import datetime
deviceAddr = 0x20

i2c = Adafruit_I2C( deviceAddr, -1, False )

#to change adress
#i2c.write8( 1, 0x22 )

#reset sensor, we need this otherwise i get inconsistent light reading in the dark...
i2c.write8( deviceAddr, 0x06 )
sleep(5)

i2c.write8(deviceAddr, 3)
sleep(3)
light = i2c.readU16(4, False)
temp = i2c.readS16(5, False)/float(10)
moisture = i2c.readU16(0, False)
print "Temperature\tMoisture\tBrightness"
print str(temp) + ":" + str(moisture) + ":" + str(light)
```

### Arduino library
Ingo Fischer has written an Arduino library for the sensor, it has a couple of ready made examples: https://github.com/Apollon77/I2CSoilMoistureSensor 

Below are old examples for bare-bones Arduino illustrating a basic I2C use.

### Arduino example
```arduino
#include <Wire.h>

void writeI2CRegister8bit(int addr, int value) {
  Wire.beginTransmission(addr);
  Wire.write(value);
  Wire.endTransmission();
}

unsigned int readI2CRegister16bit(int addr, int reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission();
  delay(20);
  Wire.requestFrom(addr, 2);
  unsigned int t = Wire.read() << 8;
  t = t | Wire.read();
  return t;
}

void setup() {
  Wire.begin();
  Serial.begin(9600);
  writeI2CRegister8bit(0x20, 6); //reset
}

void loop() {
  Serial.print(readI2CRegister16bit(0x20, 0)); //read capacitance register
  Serial.print(", ");
  Serial.print(readI2CRegister16bit(0x20, 5)); //temperature register
  Serial.print(", ");
  writeI2CRegister8bit(0x20, 3); //request light measurement 
  Serial.println(readI2CRegister16bit(0x20, 4)); //read light register
}
```
### Note for ESP8266 based systems
In some cases the default ESP8266 Arduino I2C library has the clock stretching timeout set too low. If you experience intermittent communication, add this to your code:
```
Wire.setClockStretchLimit(2500)
```

## Address change example
By default the sensor comes with 0x20 set as an address, this is an example on how to change address for indivitual sensor:
```arduino
#include <Wire.h>
 
void writeI2CRegister8bit(int addr, int reg, int value) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}
 
void writeI2CRegister8bit(int addr, int value) {
  Wire.beginTransmission(addr);
  Wire.write(value);
  Wire.endTransmission();
}
 
void setup() {
  Wire.begin();
  Serial.begin(9600);
                                       //talking to the default address 0x20
  writeI2CRegister8bit(0x20, 1, 0x21); //change address to 0x21
  writeI2CRegister8bit(0x20, 6);       //reset
  delay(1000);                         //give it some time to boot
}
 
/*loop scans I2C bus and displays foud addresses*/ 
void loop()
{
  byte error, address;
  int nDevices;
 
  Serial.println("Scanning...");
 
  nDevices = 0;
  for(address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
 
      nDevices++;
    }
    else if (error==4)
    {
      Serial.print("Unknow error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
 
  delay(5000);           // wait 5 seconds for next scan
}
```
### Particle Photon

There is a great [tutorial](ParticlePhoton-tutorial.md) by Miriam Cox for Particle Photon boards. Also there is a [library available](https://github.com/VintageGeek/I2CSoilMoistureSensor) by Mike.

## Links and mentions

* A [video from Growing Robot](https://www.youtube.com/watch?v=rB4vS7I0euA) about using the sensors with Raspberry Pi
* A [complete Open Source logging solution](https://github.com/jcw/zelkova) employing a batch of 40 sensors in Crete for tree monitoring
