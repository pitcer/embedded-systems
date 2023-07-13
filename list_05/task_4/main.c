#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <util/delay.h>

#define BAUD 9600 // baudrate
#define UBRR_VALUE ((F_CPU) / 16 / (BAUD)-1) // zgodnie ze wzorem

// inicjalizacja UART
void uart_init() {
    // ustaw baudrate
    UBRR0 = UBRR_VALUE;
    // wyczyść rejestr UCSR0A
    UCSR0A = 0;
    // włącz odbiornik i nadajnik
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);
    // ustaw format 8n1
    UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);
}

// transmisja jednego znaku
int uart_transmit(char data, FILE* stream) {
    // czekaj aż transmiter gotowy
    while (!(UCSR0A & _BV(UDRE0)))
        ;
    UDR0 = data;
    return 0;
}

// odczyt jednego znaku
int uart_receive(FILE* stream) {
    // czekaj aż znak dostępny
    while (!(UCSR0A & _BV(RXC0)))
        ;
    return UDR0;
}

// oczekiwanie na zakończenie transmisji
void uart_wait() {
    while (!(UCSR0A & _BV(TXC0)))
        ;
}

FILE uart_file;

#define TOP 15624

static void initialize_timer_1() {
    // ustaw tryb licznika
    // COM1A = 10   -- non-inverting mode
    // WGM1  = 1111 -- fast PWM top=OCR1A
    // CS1   = 101  -- prescaler 1024
    // ICR1  = 15624
    // częstotliwość 16e6/(1024*(1+15624)) = 1 Hz
    OCR1A = TOP;
    TCCR1A = _BV(COM1A1) | _BV(WGM11) | _BV(WGM10);
    TCCR1B = _BV(WGM12) | _BV(WGM13) | _BV(CS12) | _BV(CS10);
    // odmaskowanie przerwania input capture
    TIMSK1 |= _BV(ICIE1);
}

static volatile uint16_t last_timestamp = 0;
static volatile uint8_t last_period = 1;

ISR(TIMER1_CAPT_vect) {
    uint16_t timestamp = ICR1;
    last_period = timestamp - last_timestamp;
    last_timestamp = timestamp;
}

int main() {
    // zainicjalizuj UART
    uart_init();
    // skonfiguruj strumienie wejścia/wyjścia
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    initialize_timer_1();

    // ustaw tryb uśpienia na tryb bezczynności
    set_sleep_mode(SLEEP_MODE_IDLE);

    // odmaskuj przerwania
    sei();

    while (1) {
        sleep_mode();

        uint8_t period = last_period;
        uint16_t frequency = TOP / period;
        printf("Frequency: %u Hz\r\n", frequency);
        uart_wait(); // poczekaj na UART zanim się uśpisz
        _delay_us(400); // poczekaj jeszcze trochę
    }
}
