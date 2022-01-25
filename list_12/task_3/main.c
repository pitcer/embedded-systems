#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>

#define BAUD 9600 // baudrate
#define UBRR_VALUE ((F_CPU) / 16 / (BAUD)-1) // zgodnie ze wzorem

// inicjalizacja UART
static inline void uart_init(void) {
    // ustaw baudrate
    UBRR0 = UBRR_VALUE;
    // włącz odbiornik i nadajnik
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);
    // ustaw format 8n1
    UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);
}

// transmisja jednego znaku
static int uart_transmit(char data, FILE* stream) {
    // czekaj aż transmiter gotowy
    while (!(UCSR0A & _BV(UDRE0)))
        ;
    UDR0 = data;
    return 0;
}

// odczyt jednego znaku
static int uart_receive(FILE* stream) {
    // czekaj aż znak dostępny
    while (!(UCSR0A & _BV(RXC0)))
        ;
    return UDR0;
}

FILE uart_file;

#define PHOTORESISTOR_PORT PORTD
#define PHOTORESISTOR_PIN PIND
#define PHOTORESISTOR PD2

static void initialize_io() {
    // ustaw pull-up na PD2 (INT0)
    PHOTORESISTOR_PORT |= _BV(PHOTORESISTOR);
    // ustaw wyzwalanie przerwania na INT0 zboczem narastającym
    EICRA |= _BV(ISC00) | _BV(ISC01);
    // odmaskuj przerwania dla INT0
    EIMSK |= _BV(INT0);
}

static volatile uint16_t counter = 0;

ISR(INT0_vect) {
    counter++;
}

static inline void initialize_timer(void) {
    // ustaw tryb licznika
    // COM1A = 10   -- non-inverting mode
    // WGM1  = 1110 -- Fast PWM top=ICR1
    // CS1   = 101  -- prescaler 1024
    // ICR1  = 16*10^6/(frequency*1024)-1
    // częstotliwość 16e6/(1024*(1+ICR1) = frequency Hz

    TCCR1A = _BV(COM1A1);
    TCCR1B = _BV(CS12) | _BV(CS10);

    TCCR1A |= _BV(WGM11);
    TCCR1B |= _BV(WGM13) | _BV(WGM12);

    DDRB |= _BV(PB1);
}

int main(void) {
    uart_init();
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    initialize_io();

    printf("Podaj częstotliwość migania diody...\r\n");
    uint16_t frequency;
    scanf("%" SCNu16, &frequency);
    ICR1 = (uint32_t)16000000 / ((uint32_t)frequency * 1024) - 1;
    OCR1A = ICR1 / 2;
    initialize_timer();

    while (1) {
        counter = 0;

        sei();
        _delay_ms(1000);
        cli();

        printf("%" PRIu16 " Hz\r\n", counter);
    }
}
