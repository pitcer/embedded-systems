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

#define EN PB0
#define TIMER_1A PB1
#define TIMER_1B PB2
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
#define TIMER_ADC_MULTIPLIER 2

static inline void initialize_timer(void) {
    // ustaw tryb licznika
    // COM1A = 10   -- non-inverting mode
    // COM1B = 10   -- non-inverting mode
    // WGM1  = 1110 -- fast PWM top=ICR1
    // CS1   = 010  -- prescaler 8
    // ICR1  = 2046
    // częstotliwość 16e6/(8*(1+2046)) = 977 Hz
    // wzór: datasheet 20.12.3 str. 164

    ICR1 = ADC_MAXIMUM * TIMER_ADC_MULTIPLIER;
    TCCR1A = _BV(WGM11);
    TCCR1B = _BV(WGM12) | _BV(WGM13) | _BV(CS11);
    // ustaw pin OC1A i OC1B (PB1, PB2) jako wyjście
    DDRB |= _BV(TIMER_1A) | _BV(TIMER_1B);
}

int main(void) {
    // zainicjalizuj UART
    // uart_init();
    // // skonfiguruj strumienie wejścia/wyjścia
    // fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    // stdin = stdout = stderr = &uart_file;

    initialize_adc();
    initialize_timer();
    DDRB |= _BV(EN);
    PORTB |= _BV(EN);

    while (1) {
        uint16_t adc = read_adc();
        if (adc <= ADC_MAXIMUM / 2) {
            TCCR1A |= _BV(COM1A1);
            TCCR1A &= ~_BV(COM1B1);
            PORTB &= ~_BV(TIMER_1B);
            OCR1A = (ADC_MAXIMUM - adc * 2) * TIMER_ADC_MULTIPLIER;
            // printf("%u %u\r\n", adc, OCR1A);
        } else {
            TCCR1A &= ~_BV(COM1A1);
            TCCR1A |= _BV(COM1B1);
            PORTB &= ~_BV(TIMER_1A);
            OCR1B = (ADC_MAXIMUM - (ADC_MAXIMUM - adc) * 2) * TIMER_ADC_MULTIPLIER;
            // printf("%u %u\r\n", adc, OCR1B);
        }
        _delay_ms(100);
    }
}
