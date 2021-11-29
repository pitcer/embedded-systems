#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

//     16
//     __
// 18 |17| 15
//     --
//  1 |  | 3
//     --
//      2  . 4

// O7 <-> 4
// O6 <-> 17
// O5 <-> 18
// O4 <-> 1
// O3 <-> 2
// O2 <-> 3
// O1 <-> 15
// O0 <-> 16

static const uint8_t DIGITS[] = {
    0b00111111, // 0
    0b00000110, // 1
    0b11011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b11101101, // 5
    0b11111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b11101111, // 9
};

#define LA PB1
#define OE PB2

// inicjalizacja SPI
static void initialize_spi() {
    // ustaw piny MOSI, SCK i ~SS i B1 jako wyjścia
    DDRB |= _BV(DDB3) | _BV(DDB5) | _BV(OE) | _BV(LA);
    // włącz SPI w trybie master z zegarem 250 kHz
    SPCR = _BV(SPIE) | _BV(SPE) | _BV(MSTR) | _BV(SPR1);
}

ISR(SPI_STC_vect) {
    // załaduj nowy stan diód
    PORTB |= _BV(LA);
    PORTB &= ~_BV(LA);
}

#define SPI_TRANSFER(data) SPDR = data

int main() {
    PORTB |= _BV(OE);
    PORTB &= _BV(LA);

    initialize_spi();

    sei();

    SPI_TRANSFER(0xFF);
    PORTB &= ~_BV(OE);
    _delay_ms(1000);

    while (1) {
        for (uint8_t index = 0; index < 10; index++) {
            SPI_TRANSFER(DIGITS[index]);
            _delay_ms(1000);
        }
    }
}
