Getting sensor to work with Particle Photon
===========================================
by Thanks Miriam Cox

As someone completely new to electrical work and to Particle microcontrollers, I hit a few slight hurdles in trying to convert this. For anyone else interested in how I got the sensor working with a Particle Photon, it's pretty simple:

**1) STARTING YOUR PROJECT**
Copy the example code into a "New App" in your Particle.io web IDE. You can name it whatever you'd like. Make sure you ALSO have your Particle Photon microcontroller registered to your Particle account so that you can flash the project onto it.

**2) ADDING A LIBRARY**
Create a new file inside this new app, then name it according to the library you are using (I eventually used Apollon77's code, as suggested by Miceuz) and copy the appropriate code from its git repo into these new files. Don't forget to include the library in your app's main file using the following syntax:
#include "NameOfLibraryFile.h"

_Note: As of this writing, it is also possible to fork this repo and rewrite it for import as a library into your Particle app, but that is a bit more work._

**3) INCLUDES**
In any library or example files you added to your project, strip out "define if/else" statements and their contents, then replace them with the following line (which will add Wire library and device functionality):
**#include "Particle.h"**

Flash your Photon with this app. Assuming you are successful (and assuming you also have the particle software installed on your computer) you can then open up a command line prompt on your computer and enter this command to view the serial monitor output from the Photon:
**particle serial monitor**

**4) SOLDERING**
For prototyping, I unplugged the Photon (which had headers), then popped it onto a breadboard. Then I stripped some jumper wires and soldered them directly to the I2C sensor's SCK/SCL line, VCC line, GND line, and SDA/MOSI line.

Obviously, you need to connect the Photon to your sensor; so to take advantage of the I2C capabilities of the Photon, you'll need to connect the Photon's D1 input to the sensor's SCK/SCL (clock) output, and then connect the Photon's D0 input to the sensor's SDA (data) output on the breadboard. Connect the sensor's VCC line and the Photon's 3V3 (power) input together on the breadboard. Then connect the sensor's GND line and the Photon's GND (ground) lines together on the breadboard.

**VERY IMPORTANT:** You will also need "pull up resistors" on the clock and the data lines; initially, I had NO IDEA what this meant, but, it turns out you'll just connect those lines into the power line using a resistor. All I had to do was grab a resistor and connect one leg of it to the SDA/MOSI-D0 line via the breadboard, then the other leg to the VCC-3V3 (power) line on the breadboard. Then I did the same for the clock, connecting one leg of the resistor to the SCK/SCL-D1 line and the other to the VCC-3V3 line.

_Note: in my case, all I had were 200 ohm resistors, and these at least got my project working. If you don't have any resistors connected, then your particle serial monitor won't even recognize that the I2C sensor is connected. Readings on the web suggest that ideally, you should use somewhere between a 4.7k ohm and 10k ohm resistor for each pull up, to help prevent "drift" on your readings, but someone more electrically inclined than myself could explain/clarify this._

I hope this helps! This sensor is absolutely FANTASTIC, by the way. Very sensitive (which I like), uses capacitive reading (which is much more accurate), and easy to use. Happy DIYing, friends!
