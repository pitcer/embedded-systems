#include <avr/io.h>
#include <util/delay.h>

#include <stdio.h>

#define BAUD 9600 // baudrate
#define UBRR_VALUE ((F_CPU) / 16 / (BAUD)-1) // zgodnie ze wzorem

// inicjalizacja UART
static inline void uart_init() {
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

#define POTENTIOMETER_MUX ADC0D

static inline void initialize_adc(void) {
    ADMUX = _BV(REFS0); // referencja AVcc = 5V
    // pomiar ADC3
    ADMUX |= POTENTIOMETER_MUX;
    // wyłącz wejścia cyfrowe na ADC3
    DIDR0 = _BV(POTENTIOMETER_MUX);
    // częstotliwość zegara ADC 125 kHz (16 MHz / 128)
    ADCSRA = _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); // preskaler 128
    ADCSRA |= _BV(ADEN); // włącz ADC
}

static inline uint16_t read_adc(void) {
    ADCSRA |= _BV(ADSC); // wykonaj konwersję
    loop_until_bit_is_set(ADCSRA, ADIF); // czekaj na wynik
    ADCSRA |= _BV(ADIF); // wyczyść bit ADIF (pisząc 1!)
    return ADC; // weź zmierzoną wartość (0..1023)
}

#define ADC_MAXIMUM 1023

static inline void initialize_timer(void) {
    // ustaw tryb licznika
    // COM1A = 10   -- non-inverting mode
    // WGM1  = 1110 -- fast PWM top=ICR1
    // CS1   = 010  -- prescaler 8
    // ICR1  = 1249
    // częstotliwość 16e6/(8*(1+39999)) = 50 Hz
    // wzór: datasheet 20.12.3 str. 164

    ICR1 = 39999;
    TCCR1A = _BV(COM1A1) | _BV(WGM11);
    TCCR1B = _BV(WGM12) | _BV(WGM13) | _BV(CS11);
    // ustaw pin OC1A (PB1) jako wyjście
    DDRB |= _BV(PB1);
}

int main(void) {
    // zainicjalizuj UART
    // uart_init();
    // // skonfiguruj strumienie wejścia/wyjścia
    // fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    // stdin = stdout = stderr = &uart_file;

    initialize_adc();
    initialize_timer();

    while (1) {
        uint32_t adc = read_adc();
        OCR1A = 116 * adc / 33 + 1029;
        // printf("%u %u\r\n", adc, OCR1A);
        _delay_ms(10);
    }
}
