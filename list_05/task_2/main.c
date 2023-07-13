#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <stdlib.h>
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

FILE uart_file;

static void initialize_adc() {
    ADMUX = _BV(REFS0); // referencja AVcc = 5V, pomiar ADC0
    DIDR0 = _BV(ADC0D); // wyłącz wejście cyfrowe na ADC0
    // częstotliwość zegara ADC 125 kHz (16 MHz / 128)
    ADCSRA = _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); // preskaler 128
    ADCSRA |= _BV(ADATE); // włącz ADC auto-trigger
    ADCSRA |= _BV(ADIE); // włącz ADC
    // auto-trigger source = External Interrupt Request 0
    ADCSRB |= _BV(ADTS1);
    ADCSRA |= _BV(ADEN); // włącz ADC
}

#define BUTTON_PORT PORTD
#define BUTTON_PIN PIND
#define BUTTON PD2

static void initialize_io() {
    // ustaw pull-up na PD2 (INT0)
    BUTTON_PORT |= _BV(BUTTON);
    // ustaw wyzwalanie przerwania na INT0  zboczem narastającym
    EICRA |= _BV(ISC00) | _BV(ISC01);
    // odmaskuj przerwania dla INT0
    EIMSK |= _BV(INT0);
}

static volatile uint16_t last_adc_value = 0;

ISR(INT0_vect) { }

ISR(ADC_vect) {
    last_adc_value = ADC; // 0..1023
    // zapisz, że ADC ma teraz nową wartość
    last_adc_value |= _BV(15);
}

int main() {
    // zainicjalizuj UART
    uart_init();
    // skonfiguruj strumienie wejścia/wyjścia
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    initialize_adc();
    initialize_io();

    float photoresistor_resistance = 0;
    while (1) {
        cli();
        uint16_t raw_adc = last_adc_value;
        // zapisz, że ADC ma starą wartość
        uint16_t adc = raw_adc & ~_BV(15);
        last_adc_value = adc;
        sei();

        if (raw_adc & _BV(15)) {
            const float vref = 5;
            float photoresistor_voltage = (adc * vref) / 1024;
            // rezystor 10kOhm
            const float resistor_resistance = 10000;
            photoresistor_resistance = photoresistor_voltage * resistor_resistance / (vref - photoresistor_voltage);
        }

        printf("%f Ohm\r\n", photoresistor_resistance);
        _delay_ms(1000);
    }
}
