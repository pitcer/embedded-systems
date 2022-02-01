#include <avr/interrupt.h>
#include <avr/io.h>
#include <math.h>
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

#define MICROPHONE_MUX ADC1D

static inline void initialize_adc(void) {
    ADMUX = _BV(REFS0); // referencja AVcc = 5V
    ADMUX |= MICROPHONE_MUX; // pomiar ADC1
    DIDR0 = _BV(MICROPHONE_MUX); // wyłącz wejście cyfrowe na ADC1
    // częstotliwość zegara ADC 125 kHz (16 MHz / 128)
    ADCSRA = _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); // preskaler 128
    ADCSRA |= _BV(ADIE); // enable interrupt
    ADCSRA |= _BV(ADATE); // auto-trigger
    ADCSRB |= _BV(ADTS2) | _BV(ADTS1); // auto-trigger on timer 1 overflow
    ADCSRA |= _BV(ADEN); // włącz ADC
}

#define set_bit(reg, index) (reg) |= _BV(index);
#define clear_bit(reg, index) (reg) &= ~_BV(index);
#define get_bit(reg, index) ((reg)&_BV(index))

#define START_ADC_CONVERSION set_bit(ADCSRA, ADSC);

#define SAMPLES 2000

static volatile uint32_t samples_squares_sum = 0;
static volatile uint16_t samples_counter = 0;

#define OFFSET 512

ISR(ADC_vect) {
    if (samples_counter < SAMPLES) {
        int32_t adc = (int32_t)ADC - OFFSET;
        samples_squares_sum += adc * adc;
        samples_counter++;
    }
}

static inline uint32_t adc_to_milivolts(uint32_t adc) {
    const uint32_t vref = 5;
    return (adc * vref * 1000) / 1024;
}

static inline void initialize_timer(void) {
    // ustaw tryb licznika
    // COM1A = 10   -- non-inverting mode
    // WGM1  = 1110 -- Fast PWM top=ICR1
    // CS1   = 010  -- prescaler 8
    // ICR1  = 249
    // częstotliwość 16e6/(8*(1+249)) = 8000 Hz

    ICR1 = 249;
    TCCR1A = _BV(COM1A1);
    TCCR1B = _BV(CS11);

    TCCR1A |= _BV(WGM11);
    TCCR1B |= _BV(WGM13) | _BV(WGM12);

    TIMSK1 |= _BV(TOIE1);
}

ISR(TIMER1_OVF_vect) { }

int main() {
    uart_init();
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    initialize_adc();
    initialize_timer();

    sei();

    while (1) {
        if (samples_counter == SAMPLES) {
            cli();
            const uint32_t squares_sum = samples_squares_sum;
            const uint16_t counter = samples_counter;
            samples_counter = 0;
            samples_squares_sum = 0;
            sei();

            const float mean = sqrt((float)squares_sum / counter);
            const float mean_volts = (mean * 5.0) / 1024.0;
            const float x_ref = ((OFFSET - 50) * 5.0) / 1024.0;
            const float decibels = 20.0 * log10(mean_volts / x_ref);
            printf("Sum: %lu (%u); Mean: %f mV (%f); %f dBFS\r\n", squares_sum, counter, mean_volts, mean, decibels);
        }

        _delay_ms(100);
    }
}
