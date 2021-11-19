#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <util/delay.h>

#define LED PB5
#define LED_DDR DDRB
#define LED_PORT PORTB

#define BAUD 9600 // baudrate
#define UBRR_VALUE ((F_CPU) / 16 / (BAUD)-1) // zgodnie ze wzorem

// inicjalizacja UART
void uart_init() {
    // ustaw baudrate
    UBRR0 = UBRR_VALUE;
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

// inicjalizacja ADC
static void initialize_adc() {
    ADMUX = _BV(REFS0); // referencja AVcc
    ADMUX |= _BV(MUX3) | _BV(MUX2) | _BV(MUX1); // pomiar wbudowanego napięcia referencyjnego 1.1V
    // DIDR0 = _BV(ADC0D); // wyłącz wejście cyfrowe na ADC0
    // częstotliwość zegara ADC 125 kHz (16 MHz / 128)
    ADCSRA = _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); // preskaler 128
    ADCSRA |= _BV(ADEN); // włącz ADC
}

ISR(ADC_vect) {
}

FILE uart_file;

#define MEASUREMENTS 255

static float measurements[MEASUREMENTS] = { 0 };

static float calculate_average() {
    float average = 0;
    for (uint8_t index = 0; index < MEASUREMENTS; index++) {
        average += measurements[index];
    }
    return average / MEASUREMENTS;
}

#define SQUARE(x) x * x

static float calculate_variance() {
    float average = calculate_average();
    float variance = 0;
    for (uint8_t index = 0; index < MEASUREMENTS; index++) {
        variance += SQUARE(measurements[index] - average);
    }
    return average / MEASUREMENTS;
}

#define MAKE_MEASUREMENTS(trigger)                           \
    for (uint8_t index = 0; index < MEASUREMENTS; index++) { \
        trigger;                                             \
        uint16_t adc = ADC;                                  \
        const float vin = 1.1;                               \
        float vref = (vin * 1024) / adc;                     \
        measurements[index] = vref;                          \
    }

int main() {
    // zainicjalizuj UART
    uart_init();
    // skonfiguruj strumienie wejścia/wyjścia
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;
    // zainicjalizuj ADC
    initialize_adc();

    while (1) {
        ADCSRA &= ~_BV(ADIE);
        MAKE_MEASUREMENTS(
            ADCSRA |= _BV(ADSC);
            loop_until_bit_is_set(ADCSRA, ADIF);
            ADCSRA |= _BV(ADIF););
        float adc_variance = calculate_variance(measurements);

        ADCSRA |= _BV(ADIE); // ADC Interrupt Enable
        set_sleep_mode(SLEEP_MODE_ADC);
        sei();
        MAKE_MEASUREMENTS(sleep_mode(););
        float adc_noise_reduction_variance = calculate_variance(measurements);

        printf("                Variance: %f\r\n", adc_variance);
        printf("Noise reduction variance: %f\r\n\n", adc_noise_reduction_variance);

        _delay_ms(1000);
    }
}
