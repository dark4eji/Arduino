#define PIR PB4
#define LED PB3

#include <avr/io.h>
#include <util/delay.h>

int main(void) {  
  DDRB |= (1 << LED);  
  while(true) {  
    
     
      PORTB |= (1 << LED);      
  _delay_ms(1000); 
   PORTB &= ~(1 << LED);  
  }
}
