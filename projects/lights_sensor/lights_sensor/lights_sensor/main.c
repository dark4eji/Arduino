#define F_CPU 1000000L

#define PIR PB1
#define RELAY PB2
#define LED PB3

#include <avr/io.h>
#include <util/delay.h>

void switchPinState(unsigned int state, unsigned int pin) {
	if (state == 1) {
		PORTB |= (1 << pin);
		} else {
		PORTB &= ~(1 << pin);
	}
}

int main(void)
{
	DDRB |= (1 << LED);
	DDRB |= (1 << RELAY);
	
	while (1)
	{	unsigned int check = PINB & (1 << PIR);
		if (check) {
			switchPinState(1, LED);
			switchPinState(0, RELAY);
			} else if (!check) {
			switchPinState(0, LED);
			switchPinState(1, RELAY);
		}
	}
}

