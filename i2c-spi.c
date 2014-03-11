/*
 * i2c_spi.c
 *
 * I2C to SPI bridge for LMH6518. Very simple. Probably doesn't work under all conditions of I2C weirdness.
 * Created: 3/9/2014 22:57:51
 *  Author: bunnie
 */ 

#define F_CPU 8000000

#include <avr/io.h>
#include <util/delay.h>


#define SCL PB0
#define SDA PB1

#define SCLK PB2
#define SCS  PB3
#define SDIO PB4

#define SET_SDA_IN() DDRB &= ~(1 << SDA)
#define SET_SDA_OUT() DDRB |= (1 << SDA)
//#define READ_SDA() (PINB & (1 << SDA)) ? 1 : 0
#define READ_SDA() ((PINB & 2) ? 1 : 0)
//#define READ_SCL() (PINB & (1 << SCL)) ? 1 : 0
#define READ_SCL() (PINB & 1)

#define I2C_READ 1
#define I2C_WRITE 0

#define WAIT_SCL_RISE()   while(READ_SCL());  while(!READ_SCL());

#define MY_ADR 0x28

void relay_pin(char value) {
	if( value )	
		PORTB |= (1 << SDIO);
	else
		PORTB &= ~(1 << SDIO);
}

void blink() {
	while(1) {
		PORTB |= (1 << SDIO);
		_delay_ms(500);
		PORTB &= ~(1 << SDIO);
		_delay_ms(500);
	}
	
}

void blink_num(unsigned char num, unsigned int count) {
	int i;
	while(count--) {
		PORTB &= ~(1 << SDIO);
		_delay_ms(4000);
		
		for( i = 7; i >= 0; i-- ) {
			if( (num >> i) & 1 ) {
				PORTB |= (1 << SDIO);
				_delay_ms(250);
				PORTB &= ~(1 << SDIO);
			} else {
				PORTB |= (1 << SDIO);
				_delay_ms(750);
				PORTB &= ~(1 << SDIO);
			}
			_delay_ms(1000);
		}
	}	
}
void wait_stop() {

	WAIT_SCL_RISE();

	while(1) {
		if(READ_SDA() == 0)	{
			while(READ_SCL() == 1) {
				if(READ_SDA() == 1)
					return;
			}
		} else {
			while(READ_SCL() == 1)
				;
		}
		while(READ_SCL() == 0)
			;
	}
}

int get_address() {
	unsigned char bus;
	unsigned char bit;

	///// wait for start
	bus = 0;
	while((PINB & 3) != 0x1)  // wait for start condition
	;
	
	//// grab in I2C
	bit = 0;
	while(bit < 8) {
		bus <<= 1;
		WAIT_SCL_RISE();
		bus |= READ_SDA();
		bit++;
	}
	
//	if(bus != ((MY_ADR << 1) | I2C_WRITE)) {  // 0x50
	if((bus & 0xFE) != 0x50) {
		wait_stop();
		return 0;
	}

	//// acknowledge
	while(READ_SCL() == 1) // wait until SCL drops
	;
	SET_SDA_OUT();
	PORTB &= ~(1 << SDA); // drive it low
	
	while(READ_SCL() == 0) // wait until SCL rises
	;
	while(READ_SCL() == 1) // wait until SCL drops
	;
	SET_SDA_IN();
	//// acknowledge done	
	
	return 1;
}

int get_data(char ack) {
	unsigned char bus;
	unsigned char bit;

	bus = 0;
#if 0
	///// wait for start
	while((PINB & 3) != 0x1)  // wait for start condition
	;
#endif

	//// grab in I2C
	bit = 0;
	while(bit < 8) {
		bus <<= 1;
		WAIT_SCL_RISE();
		bus |= READ_SDA();
		bit++;
	}
	
	//// acknowledge
	while(READ_SCL() == 1) // wait until SCL drops
	;
	SET_SDA_OUT();
	if(ack)
		PORTB &= ~(1 << SDA); // drive it low
	
	while(READ_SCL() == 0) // wait until SCL rises
	;
	while(READ_SCL() == 1) // wait until SCL drops
	;
	SET_SDA_IN();
	//// acknowledge done

	return bus;
}

#define SET_SCS PORTB |= (1 << SCS)
#define CLR_SCS PORTB &= ~(1 << SCS)
#define SET_SDIO PORTB |= (1 << SDIO)
#define CLR_SDIO PORTB &= ~(1 << SDIO)
#define SET_SCLK PORTB |= (1 << SCLK)
#define CLR_SCLK PORTB &= ~(1 << SCLK)

void spi_bit(unsigned char bit) {
	if( bit )
		SET_SDIO;
	else
		CLR_SDIO;
	SET_SCLK;   // extra high time to meet duty cycle min time reqs
	SET_SCLK;
	SET_SCLK;
	SET_SCLK;
	SET_SCLK;
	SET_SCLK;
	SET_SCLK;
	SET_SCLK;
	CLR_SCLK;
}

void write_spi(unsigned short data) {
	int i;
	unsigned short dummy;
	
	dummy = 0;
	spi_bit(0); // dummy clock

	CLR_SCS;
	for( i = 7; i >= 0; i-- ) {
		spi_bit( (dummy >> i) & 0x1 );  // send "write" command	
	}
	for( i = 15; i >= 0; i-- ) {
		spi_bit( (data >> i) & 0x1 );
	}
	SET_SCS;

	spi_bit(0); // dummy clock
}

int main(void)
{
	unsigned char dataOne;
	unsigned char dataTwo;
	unsigned short data;
	
	CLKPR = 0;
	CLKPR = 8;
	
	PORTB = 0;
	DDRB = 0;
	
	PORTB |= (1 << SCS); // set CS high to make all other signals ignored
	
	DDRB |= (1 << SCLK); // setup SPI as outputs
	DDRB |= (1 << SCS);
	DDRB |= (1 << SDIO);
	
	DDRB &= ~(1 << SCL);  // set SCL as input
	SET_SDA_IN();
		
	//DDRB = 0;
	
	while((PINB & 3) != 0x3)  // wait until idle condition
		;
//relay_pin(PINB & 2)
	
	while(1) {
		while( !get_address() )
			; // routine to wait until we've been addressed
	
		dataOne = get_data(1);
		dataTwo = get_data(1); // assert nack on last data write
		
//		blink_num(dataOne, 1);
		
//		blink_num(dataTwo, 1);
		data = ((dataOne & 0xff) << 8) | (dataTwo & 0xFF);
		write_spi(data);
		// then here we would bitbang to SPI
	}
}