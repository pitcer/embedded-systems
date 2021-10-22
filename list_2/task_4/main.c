#include <avr/io.h>
#include <inttypes.h>
#include <util/delay.h>

//     16
//     __
// 18 |17| 15
//     --
//  1 |  | 3
//     --
//      2  . 4

// PD0 <-> 4
// PD1 <-> 3
// PD2 <-> 2
// PD3 <-> 1
// PD4 <-> 18
// PD5 <-> 17
// PD6 <-> 16
// PD7 <-> 15

static const uint8_t TENTHS_DIGITS[] = {
    ~0x0, // off
    ~0b10000010, // 1
    ~0b11101100, // 2
    ~0b11100110, // 3
    ~0b10110010, // 4
    ~0b01110110, // 5
};

static const uint8_t DIGITS[] = {
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

#define DISPLAY_NUMBERS(first, second, repeat)         \
    for (uint8_t index = 0; index < repeat; index++) { \
        PORTC |= _BV(PC1);                             \
        PORTC &= ~_BV(PC0);                            \
        PORTD = first;                                 \
        _delay_ms(2);                                  \
        PORTC |= _BV(PC0);                             \
        PORTC &= ~_BV(PC1);                            \
        PORTD = second;                                \
        _delay_ms(2);                                  \
    }

#define REPEAT_SECOND 250

int main() {
    UCSR0B &= ~_BV(RXEN0) & ~_BV(TXEN0);

    DDRC |= _BV(PC0);
    DDRC |= _BV(PC1);
    PORTC |= _BV(PC0); // wyłącz przepływ przez tranzystor
    PORTC |= _BV(PC1); // wyłącz przepływ przez tranzystor

    DDRD = 0xFF;

    DISPLAY_NUMBERS(~0xFF, ~0xFF, REPEAT_SECOND);

    while (1) {
        for (uint8_t first_digit = 0; first_digit < 6; first_digit++) {
            for (uint8_t second_digit = 0; second_digit < 10; second_digit++) {
                DISPLAY_NUMBERS(
                    TENTHS_DIGITS[first_digit],
                    DIGITS[second_digit],
                    REPEAT_SECOND);
            }
        }
    }
}
