#include <avr/io.h>
#include <inttypes.h>
#include <util/delay.h>

#define DELAY 100

int main() {
    UCSR0B &= ~_BV(RXEN0) & ~_BV(TXEN0);

    DDRD |= 0b11111111;

    uint8_t index = 0;
    while (1) {
        for (; index < 7; index++) {
            PORTD = _BV(index);
            _delay_ms(DELAY);
        }

        for (; index > 0; index--) {
            PORTD = _BV(index);
            _delay_ms(DELAY);
        }
    }
}
