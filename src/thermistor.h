#ifndef __THERMISTOR_H
#define __THERMISTOR_H
/**
 * Returns temperature as a function of 10bit ADC value
 **/
long thermistorLsbToTemperature(unsigned int lsb);

#endif