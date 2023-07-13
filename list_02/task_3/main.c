#include <avr/io.h>
#include <inttypes.h>
#include <util/delay.h>

#define BUTTON_PORT PORTC
#define BUTTON_PIN PINC
#define NEXT_BUTTON PC2
#define PREVIOUS_BUTTON PC0
#define RESET_BUTTON PC1

uint8_t to_gray(uint8_t n) {
    return n ^ (n >> 1);
}

#define HANDLE_BUTTON(button, counter_handler)          \
    if (!((BUTTON_PIN | pressed_mask) & _BV(button))) { \
        _delay_ms(40);                                  \
        if (!(BUTTON_PIN & _BV(button))) {              \
            counter counter_handler;                    \
            pressed_mask |= _BV(button);                \
        }                                               \
    }                                                   \
    if (BUTTON_PIN & pressed_mask & _BV(button)) {      \
        _delay_ms(40);                                  \
        if (BUTTON_PIN & _BV(button)) {                 \
            pressed_mask &= ~_BV(button);               \
        }                                               \
    }

int main() {
    UCSR0B &= ~_BV(RXEN0) & ~_BV(TXEN0);

    BUTTON_PORT |= _BV(RESET_BUTTON);
    BUTTON_PORT |= _BV(NEXT_BUTTON);
    BUTTON_PORT |= _BV(PREVIOUS_BUTTON);

    DDRD = 0xFF;
    PORTD = 0x00;

    uint8_t pressed_mask = 0x0;
    uint8_t counter = 0;
    while (1) {
        PORTD = to_gray(counter);
        HANDLE_BUTTON(PREVIOUS_BUTTON, -= 1);
        HANDLE_BUTTON(NEXT_BUTTON, += 1);
        HANDLE_BUTTON(RESET_BUTTON, = 0);
    }
}
