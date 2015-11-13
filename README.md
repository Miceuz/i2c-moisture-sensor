# i2c-moisture-sensor

I2C based soil moisture sensor. A continuation of the [Chirp - plant watering alarm](https://github.com/Miceuz/PlantWateringAlarm) project. There is also an [RS485](https://github.com/Miceuz/rs485-moist-sensor) and an [analog](https://github.com/Miceuz/soil-moisture-sensor-analog) version available.

##I2C protocol
Available registers for reading and writing.

Name | Register | R/W | Data length 
-----|----------|-----|-------------
GET_CAPACITANCE   | 0x00 | (r) | 2
SET_ADDRESS       | 0x01 | (w) | 1
GET_ADDRESS       | 0x02 | (r) | 1
MEASURE_LIGHT     | 0x03 | (w) | 0
GET_LIGHT         | 0x04 | (r) | 2
GET_TEMPERATURE   | 0x05 | (r) | 2
RESET             | 0x06 | (w) | 0
GET_VERSION       | 0x07 | (r) | 1

###Raspberry Pi example
This is a RasPi example provided by user *krikk*
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
print str(temp) + ":" + str(moisture) + ":" + str(light)
```

###Arduino library
Ingo Fischer has written an Arduino library for the sensor, it has a couple of ready made examples: https://github.com/Apollon77/I2CSoilMoistureSensor 

Below are old examples for bare-bones Arduino illustrating a basic I2C use.

###Arduino example
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

##Address change example
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

