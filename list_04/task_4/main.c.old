#include <avr/io.h>
#include <util/delay.h>

#define LED PB5
#define LED_DDR DDRB
#define LED_PORT PORTB

#define DETECTOR PB0
#define DETECTOR_PIN PINB
#define DETECTOR_PORT PORTB

void initialize_timer() {
    // ustaw tryb licznika
    // COM1A = 10   -- non-inverting mode
    // WGM1  = 1110 -- fast PWM top=ICR1
    // CS1   = 001  -- prescaler 1
    // ICR1  = 421
    // chcemy ....
    // częstotliwość 16e6/(1024*(1+15624)) = 1 Hz
    // wzór: datasheet 20.12.3 str. 164
    ICR1 = 421;
    TCCR1A = _BV(COM1A1) | _BV(WGM11);
    TCCR1B = _BV(WGM12) | _BV(WGM13) | _BV(CS10);
    // ustaw pin OC1A (PB1) jako wyjście
    DDRB |= _BV(PB1);
}

int main() {
    DETECTOR_PORT |= _BV(DETECTOR);
    LED_DDR |= _BV(LED);
    initialize_timer();
    // ustaw wypełnienie 50%
    OCR1A = ICR1 / 2;

    // TEMP
    // DDRB |= _BV(PB1);
    // PORTB |= _BV(PB1);
    // END TEMP

    while (1) {

        if (DETECTOR_PIN & _BV(DETECTOR)) {
            LED_PORT &= ~_BV(LED);
        } else {
            LED_PORT |= _BV(LED);
        }

        for (uint8_t i = 0; i < 5; i++) {
            // ENABLE TIMER
            TCCR1A |= _BV(COM1A1);

            _delay_us(600);

            // DISABLE TIMER
            TCCR1A &= ~_BV(COM1A1);

            _delay_us(600);
        }
        // ENABLE TIMER
        TCCR1A |= _BV(COM1A1);

        _delay_us(600);

        // DISABLE TIMER
        TCCR1A &= ~_BV(COM1A1);

        if (DETECTOR_PIN & _BV(DETECTOR)) {
            LED_PORT &= ~_BV(LED);
        } else {
            LED_PORT |= _BV(LED);
        }

        _delay_us(100000 - (2 * 600 * 5 + 600));
    }
}
