#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>

#define BAUD 9600 // baudrate
#define UBRR_VALUE ((F_CPU) / 16 / (BAUD)-1) // zgodnie ze wzorem

// inicjalizacja UART
static inline void initialize_uart() {
    // ustaw baudrate
    UBRR0 = UBRR_VALUE;
    // wyczyść USART Data Register Empty
    UCSR0A &= ~_BV(UDRE0);
    UCSR0B |= _BV(RXCIE0); // receive complete interrupt enable
    // UCSR0B |= _BV(TXCIE0); // transmit complete interrupt enable
    // włącz odbiornik i nadajnik
    UCSR0B |= _BV(RXEN0); // enable receiver
    UCSR0B |= _BV(TXEN0); // enable transmitter
    UCSR0C = _BV(UCSZ00) | _BV(UCSZ01); // character size 8 bits; set by default
}

ISR(USART_RX_vect) {
    UDR0 = UDR0;
}

int main() {
    initialize_uart();

    set_sleep_mode(SLEEP_MODE_IDLE);

    sei();

    while (1) {
        sleep_mode();
    }
}
