#ifndef __TWI_H
#define __TWI_H

#define TWI_RX_BUFFER_SIZE  ( 16 )
#define TWI_TX_BUFFER_SIZE ( 16 )

void twiSetup(unsigned char address);
char twiDataInReceiveBuffer(void);
unsigned char twiReceiveByte(void);
void twiTransmitByte(unsigned char data);
char twiIsValidAddress(unsigned char address);

#endif