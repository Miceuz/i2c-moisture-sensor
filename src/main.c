#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
//#include <avr/wdt.h>
#include <avr/sleep.h>
#include "thermistor.h"
#include "twi.h"

#define FIRMWARE_VERSION 0x26 //2.6

#define LED_K PA1
#define LED_A PA0
#define LED_DDR DDRA
#define LED_PORT PORTA
#define POWER_DDR DDRA
#define POWER_PORT PORTA
#define POWER_PIN PA2

#define CHANNEL_THERMISTOR 3
#define CHANNEL_CAPACITANCE_HIGH 5
#define CHANNEL_CAPACITANCE_LOW 7
#define CHANNEL_CHIP_TEMP 0b00001100

#define TWI_GET_CAPACITANCE     0x00
#define TWI_SET_ADDRESS         0x01
#define TWI_GET_ADDRESS         0x02
#define TWI_MEASURE_LIGHT       0x03
#define TWI_GET_LIGHT           0x04
#define TWI_GET_TEMPERATURE     0x05
#define TWI_RESET               0x06
#define TWI_GET_VERSION         0x07
#define TWI_SLEEP               0x08
#define TWI_GET_BUSY            0x09

#define I2C_ADDRESS_EEPROM_LOCATION (uint8_t*)0x01
#define I2C_ADDRESS_DEFAULT         0x20

inline static void ledSetup(){
    LED_DDR |= _BV(LED_A) | _BV(LED_K);
    LED_PORT &= ~_BV(LED_A);
    LED_PORT &= ~_BV(LED_K);
}

inline static void ledOn() {
    LED_PORT |= _BV(LED_A);
}

inline static void ledOff() {
    LED_PORT &= ~_BV(LED_A);
}

inline static void powerOn() {
	POWER_DDR |= _BV(POWER_PIN);
	POWER_PORT |= _BV(POWER_PIN);
}

inline static void powerOff() {
	POWER_PORT &= ~_BV(POWER_PIN);
}

inline static void ledToggle() {
    LED_PORT ^= _BV(LED_A);
}

inline static void adcSetup() {
    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
    ADCSRA |= _BV(ADIE);
    ADMUXB = 0;
}

uint8_t adcInProgress = 0;

inline static void sleepWhileADC() {
    adcInProgress = 1;
    while(adcInProgress) {
	    set_sleep_mode(SLEEP_MODE_ADC);
	    sleep_mode();
    }
}

ISR(ADC_vect) { 
	adcInProgress = 0;
    //nothing, just wake up
}

uint16_t adcReadChannel(uint8_t channel) {
    ADMUXA = channel;
    ADCSRA |= _BV(ADSC);
    sleepWhileADC();
   // loop_until_bit_is_clear(ADCSRA, ADSC);
    uint16_t ret = ADC;
    return ret;
}

uint8_t tempMeasurementInProgress = 0;

int getTemperature() {
    tempMeasurementInProgress = 1;
    uint16_t thermistor = adcReadChannel(CHANNEL_THERMISTOR);
    long temp = thermistorLsbToTemperature(thermistor);
    tempMeasurementInProgress = 0;
    return (int)temp;
}

#define isTemperatureMeasurementInProgress() tempMeasurementInProgress

uint8_t capMeasurementInProgress = 0;

uint16_t getCapacitance() {
    capMeasurementInProgress = 1;
    uint16_t caph = adcReadChannel(CHANNEL_CAPACITANCE_HIGH);
    uint16_t capl = adcReadChannel(CHANNEL_CAPACITANCE_LOW);
    capMeasurementInProgress = 0;
    return 1023 - (caph - capl);
}

#define isCapacitanceMeasurementInProgress() capMeasurementInProgress 
//--------------------- light measurement --------------------

volatile uint16_t lightCounter = 0;
volatile uint8_t lightCycleOver = 1;

#define PCINT1 1

static inline void stopLightMeasurement() {
    GIMSK &= ~_BV(PCIE0);
    TCCR1B = 0;
    PCMSK0 &= ~_BV(PCINT1);
    TIMSK1 &= ~_BV(TOIE1);

    lightCycleOver = 1;
}

ISR(PCINT0_vect) {
    GIMSK &= ~_BV(PCIE0);//disable pin change interrupts
    TCCR1B = 0;          //stop timer
    lightCounter = TCNT1;
    
    stopLightMeasurement();
}

ISR(TIMER1_OVF_vect) {
    lightCounter = 65535;
    stopLightMeasurement();
}

static inline uint16_t getLight() {
    PRR &= ~_BV(PRTIM1);

    TIMSK1 |= _BV(TOIE1);               //enable timer overflow interrupt
    
    LED_DDR |= _BV(LED_A) | _BV(LED_K);    //forward bias the LED
    LED_PORT &= ~_BV(LED_K);               //flash it to discharge the PN junction capacitance
    LED_PORT |= _BV(LED_A);

    LED_PORT |= _BV(LED_K);                //reverse bias LED to charge capacitance in it
    LED_PORT &= ~_BV(LED_A);
    _delay_us(100);
    LED_DDR &= ~_BV(LED_K);                //make Cathode input
    LED_PORT &= ~(_BV(LED_A) | _BV(LED_K));//disable pullups
    
    TCNT1 = 0;
    TCCR1A = 0;
    TCCR1B = _BV(CS10) | _BV(CS11);                 //start timer1 with prescaler clk/64
    
    PCMSK0 |= _BV(PCINT1);              //enable pin change interrupt on LED_K
    GIMSK |= _BV(PCIE0); 

    lightCycleOver = 0;

    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_mode();
}


static inline uint8_t isLightMeasurementInProgress() {
    return !lightCycleOver;
}

inline static void wdt_disable() {
    MCUSR = 0;
    CCP = 0xD8;
    WDTCSR &= ~_BV(WDE);
}

inline static void wdt_enable() {
    CCP = 0xD8;
    WDTCSR = _BV(WDE);
}

#define reset() wdt_enable(); while(1) {}

uint16_t currCapacitance = 0;
int temperature = 0;

uint8_t setAddressActive = 0;
uint8_t addressCandidate = 0;

static inline void loopSensorMode() {
    while(1) {
        if(twiDataInReceiveBuffer()) {
            uint8_t usiRx = twiReceiveByte();

            if(TWI_SET_ADDRESS != usiRx) {
                setAddressActive = 0;
                ledOff();
            }

            if(TWI_GET_CAPACITANCE == usiRx) {
                twiTransmitByte(currCapacitance >> 8);
                twiTransmitByte(currCapacitance &0x00FF);
                currCapacitance = getCapacitance();
            } else if(TWI_SET_ADDRESS == usiRx) {
                uint8_t newAddress = twiReceiveByte();

                if(twiIsValidAddress(newAddress)) {
                    if(0 == setAddressActive) {
                        ledOn();
                        setAddressActive = 1;
                        addressCandidate = newAddress;
                    } else {
                        if(newAddress == addressCandidate) {
                            eeprom_write_byte((uint8_t*)0x01, newAddress);
                        }
                        setAddressActive = 0;
                        ledOff();
                    }
                }

            } else if(TWI_GET_ADDRESS == usiRx) {
                uint8_t newAddress = eeprom_read_byte((uint8_t*) 0x01);
                twiTransmitByte(newAddress);

            } else if(TWI_MEASURE_LIGHT == usiRx) {
                if(!isLightMeasurementInProgress()) {
                    getLight();
                }

            } else if(TWI_GET_LIGHT == usiRx) {
                GIMSK &= ~_BV(PCIE0);//disable pin change interrupts
                TCCR1B = 0;          //stop timer
                
                twiTransmitByte(lightCounter >> 8);
                twiTransmitByte(lightCounter & 0x00FF);

                GIMSK |= _BV(PCIE0); 
                TCCR1B = _BV(CS10) | _BV(CS11);                 //start timer1 with prescaler clk/64
            } else if(TWI_GET_TEMPERATURE == usiRx) {
                twiTransmitByte(temperature >> 8);
                twiTransmitByte(temperature & 0x00FF);
                temperature = getTemperature();
            } else if(TWI_RESET == usiRx) {
                reset();

            } else if(TWI_GET_VERSION == usiRx) {
                twiTransmitByte(FIRMWARE_VERSION);
            } else if(TWI_SLEEP == usiRx) {
            	powerOff();
                set_sleep_mode(SLEEP_MODE_PWR_DOWN);
                sleep_mode();
                powerOn();
            } else if(TWI_GET_BUSY == usiRx) {
                twiTransmitByte(isCapacitanceMeasurementInProgress() || 
                                isTemperatureMeasurementInProgress() || 
                                isLightMeasurementInProgress());
            }
        }
    }
}

static inline void setupPowerSaving() {
    PRR = _BV(PRTIM0) | _BV(PRTIM1) | _BV(PRTIM2) | _BV(PRSPI) | _BV(PRUSART0) | _BV(PRUSART1); //shut down everything we don't use
    ACSR0A = _BV(ACD0); //disable comparators
    ACSR1A = _BV(ACD1);
    DIDR0 = _BV(ADC3D) | _BV(ADC5D) | _BV(ADC7D);//disable input buffers for analog pins
}

int main (void) {
    wdt_disable();

    uint8_t address = eeprom_read_byte(I2C_ADDRESS_EEPROM_LOCATION);
    if(!twiIsValidAddress(address)) {
        address = I2C_ADDRESS_DEFAULT;
    }

    setupPowerSaving();
    ledSetup();
    adcSetup();
    powerOn();
    sei();

    ledOn();
    _delay_ms(100);
    temperature = getTemperature();
    currCapacitance = getCapacitance();
    //we do it two times because the first reading after reset might be off...
    temperature = getTemperature();
    currCapacitance = getCapacitance();
    ledOff();

    twiSetup(address);    
    loopSensorMode();
}
