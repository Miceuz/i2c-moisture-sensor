#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
//#include <avr/wdt.h>
#include "sleep.h"
#include "thermistor.h"
#include "twi.h"

#define FIRMWARE_VERSION 0x22 //2.2

#define LED_K PA1
#define LED_A PA0
#define LED_DDR DDRA
#define LED_PORT PORTA

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

inline static ledToggle() {
    LED_PORT ^= _BV(LED_A);
}

static inline void adcSetup() {
    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS0);
    ADCSRA |= _BV(ADIE);
    ADMUXB = 0;
}

void inline sleepWhileADC() {
    set_sleep_mode(SLEEP_MODE_ADC);
    sleep_mode();
}

ISR(ADC_vect) { 
    //nothing, just wake up
}

uint16_t adcReadChannel(uint8_t channel) {
    ADMUXA = channel;
    ADCSRA |= _BV(ADSC);
    sleepWhileADC();
//    loop_until_bit_is_clear(ADCSRA, ADSC);
    uint16_t ret = ADC;
    return ret;
}

int getTemperature() {
    uint16_t thermistor = adcReadChannel(CHANNEL_THERMISTOR);
    long temp = thermistorLsbToTemperature(thermistor);
    return (int)temp;
}

uint16_t getCapacitance() {
    uint16_t caph = adcReadChannel(CHANNEL_CAPACITANCE_HIGH);
    uint16_t capl = adcReadChannel(CHANNEL_CAPACITANCE_LOW);
    return 1024 - (caph - capl);
}

//--------------------- light measurement --------------------

volatile uint16_t lightCounter = 0;
volatile uint8_t lightCycleOver = 1;
volatile uint16_t light = 0;

#define PCINT1 1

static inline stopLightMeaseurement() {
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
    light = lightCounter;
    
    stopLightMeaseurement();
}

ISR(TIMER1_OVF_vect) {
    lightCounter = 65535;
    light = lightCounter;
    stopLightMeaseurement();
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
}


static inline uint8_t lightMeasurementInProgress() {
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

static inline loopSensorMode() {
    while(1) {
        if(twiDataInReceiveBuffer()) {
            uint8_t usiRx = twiReceiveByte();

            if(TWI_GET_CAPACITANCE == usiRx) {
                twiTransmitByte(currCapacitance >> 8);
                twiTransmitByte(currCapacitance &0x00FF);
                currCapacitance = getCapacitance();
                set_sleep_mode(SLEEP_MODE_PWR_SAVE);
                sleep_mode();
            } else if(TWI_SET_ADDRESS == usiRx) {
                uint8_t newAddress = twiReceiveByte();
                if(twiIsValidAddress(newAddress)) {
                    eeprom_write_byte((uint8_t*)0x01, newAddress);
                }

            } else if(TWI_GET_ADDRESS == usiRx) {
                uint8_t newAddress = eeprom_read_byte((uint8_t*) 0x01);
                twiTransmitByte(newAddress);

            } else if(TWI_MEASURE_LIGHT == usiRx) {
                if(!lightMeasurementInProgress()) {
                    getLight();
                }

            } else if(TWI_GET_LIGHT == usiRx) {
                GIMSK &= ~_BV(PCIE0);//disable pin change interrupts
                TCCR1B = 0;          //stop timer
                
                twiTransmitByte(lightCounter >> 8);
                twiTransmitByte(lightCounter & 0x00FF);

                GIMSK |= _BV(PCIE0); 
                TCCR1B = _BV(CS10) | _BV(CS11);                 //start timer1 with prescaler clk/64
                set_sleep_mode(SLEEP_MODE_PWR_SAVE);
                sleep_mode();

            } else if(TWI_GET_TEMPERATURE == usiRx) {
                twiTransmitByte(temperature >> 8);
                twiTransmitByte(temperature & 0x00FF);
                temperature = getTemperature();
                set_sleep_mode(SLEEP_MODE_PWR_SAVE);
                sleep_mode();
            } else if(TWI_RESET == usiRx) {
                reset();

            } else if(TWI_GET_VERSION == usiRx) {
                twiTransmitByte(FIRMWARE_VERSION);
                set_sleep_mode(SLEEP_MODE_PWR_SAVE);
                sleep_mode();
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
    sei();

    ledOn();
    temperature = getTemperature();
    currCapacitance = getCapacitance();
    _delay_ms(100);
    ledOff();

    // getLight();

    // while(!lightCycleOver) {

    // }
//    _delay_ms(100);
    // ledSetup();
    // ledOn();
    // _delay_ms(100);
    // ledOff();

    twiSetup(address);    
    loopSensorMode();
}
