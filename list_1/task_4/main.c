#include <avr/io.h>
#include <inttypes.h>
#include <util/delay.h>

//     12
//     __
// 10 |11| 13
//     --
//  1 |  | 3
//     --
//      2  . 4

// PD0 <-> 4
// PD1 <-> 3
// PD2 <-> 2
// PD3 <-> 1
// PD4 <-> 10
// PD5 <-> 11
// PD6 <-> 12
// PD7 <-> 13

static const uint8_t numbers[10] = {
    ~0b11011110, // 0
    ~0b10000010, // 1
    ~0b11101100, // 2
    ~0b11100110, // 3
    ~0b10110010, // 4
    ~0b01110110, // 5
    ~0b01111111, // 6
    ~0b11000010, // 7
    ~0b11111110, // 8
    ~0b11110111, // 9
};

int main() {
    UCSR0B &= ~_BV(RXEN0) & ~_BV(TXEN0);

    DDRD = 0xFF;
    PORTD = 0x00;
    _delay_ms(1000);

    while (1) {
        for (uint8_t index = 0; index < 10; index++) {
            PORTD = numbers[index];
            _delay_ms(1000);
        }
    }
}
