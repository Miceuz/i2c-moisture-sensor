#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/delay.h>
#include "twi.h"

#define TWI_TX_BUFFER_MASK ( TWI_TX_BUFFER_SIZE - 1 )
#define TWI_RX_BUFFER_MASK  ( TWI_RX_BUFFER_SIZE - 1 )

#define I2C_DATA_INTERRUPT      0x80
#define I2C_BUS_COLLISION       0x08  
#define I2C_ADDRESS_STOP_MATCH  0x40 

static uint8_t          rxBuf[ TWI_RX_BUFFER_SIZE ];
static volatile uint8_t rxHead = 0;
static volatile unsigned char rxTail = 0;

static unsigned char          txBuf[ TWI_TX_BUFFER_SIZE ];
static volatile unsigned char txHead = 0;
static volatile unsigned char txTail = 0;

void twiSetup(unsigned char address) {
    TWSA = address << 1;
    TWSCRA = (1 << TWEN)   // Two-Wire Interface Enable
//            | (1 << TWSHE)  // TWI SDA Hold Time Enable
            | (1 << TWASIE) // TWI Address/Stop Interrupt Enable  
//            | (1 << TWSIE)  // TWI Stop Interrupt Enable
            | (1 << TWDIE); // TWI Data Interrupt Enable  
}

#define isDataInterrupt(status) status & I2C_DATA_INTERRUPT
#define isMasterRead() TWSSRA & (1 << TWDIR)
#define isAddressInterrupt() TWSSRA & (1<<TWAS)

ISR(TWI_SLAVE_vect){  
    unsigned char status = TWSSRA & 0xC0;
    if (isDataInterrupt(status)) { 
        if (isMasterRead()){
            if ( txHead != txTail ) {
                txTail = ( txTail + 1 ) & TWI_TX_BUFFER_MASK;
                TWSD = txBuf[ txTail ];
            } else {
                //what is the correct behaviour here?

                // the buffer is empty
                  TWSD = 255;
                //SET_USI_TO_TWI_START_CONDITION_MODE( );
                //return;
            }
            TWSCRB = (uint8_t) ((1<<TWCMD1)|(1<<TWCMD0));
        } else {//master write
            TWSCRB |= (uint8_t) ((1<<TWCMD1)|(1<<TWCMD0));
            rxHead = ( rxHead + 1 ) & TWI_RX_BUFFER_MASK;
            rxBuf[ rxHead ] = TWSD;
        }
    } else if (status & I2C_ADDRESS_STOP_MATCH) {    
        if (TWSSRA & I2C_BUS_COLLISION) {
            TWSCRB = (uint8_t) (1<<TWCMD1);
        } else {    
            if (isAddressInterrupt()) {
                // ACK 
                TWSCRB = (uint8_t) ((1<<TWCMD1)|(1<<TWCMD0));
            } else {// Stop Condition
                TWSCRB = (uint8_t) (1<<TWCMD1);
            }
        }
    }    
}

char twiDataInReceiveBuffer(void) {
  return rxHead != rxTail;
}

unsigned char twiReceiveByte(void){
  uint8_t waittime = 0;
  while ( rxHead == rxTail && waittime++ < 100){
    _delay_ms(1);
  }

  if(waittime < 100) {
    rxTail = ( rxTail + 1 ) & TWI_RX_BUFFER_MASK;
  }
  return rxBuf[ rxTail ];
}

void twiTransmitByte(unsigned char data) {
  unsigned char tmphead;
  tmphead = ( txHead + 1 ) & TWI_TX_BUFFER_MASK;
  while ( tmphead == txTail );
  txBuf[ tmphead ] = data;
  txHead = tmphead;
}

char twiIsValidAddress(unsigned char address) {
    return address > 0x07 && address <= 0x77;
}