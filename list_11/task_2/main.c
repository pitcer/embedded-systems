#include "pid.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>

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

#define POTENTIOMETER_MUX ADC1D
#define MOTOR_MUX ADC2D

static inline void initialize_adc(void) {
    ADMUX = _BV(REFS0); // referencja AVcc = 5V
    // wyłącz wejścia cyfrowe na ADC3
    DIDR0 = _BV(POTENTIOMETER_MUX) | _BV(MOTOR_MUX);
    // częstotliwość zegara ADC 125 kHz (16 MHz / 128)
    ADCSRA = _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); // preskaler 128
    ADCSRA |= _BV(ADEN); // włącz ADC
    ADCSRA |= _BV(ADIE); // interrupt enabled
}

static inline uint16_t read_adc(void) {
    ADCSRA |= _BV(ADSC); // wykonaj konwersję
    loop_until_bit_is_set(ADCSRA, ADIF); // czekaj na wynik
    ADCSRA |= _BV(ADIF); // wyczyść bit ADIF (pisząc 1!)
    return ADC; // weź zmierzoną wartość (0..1023)
}

static inline uint16_t adc_to_milivolts(uint32_t adc) {
    const uint32_t vref = 5;
    return (adc * vref * 1000) / 1024;
}

#define ADC_MAXIMUM 1023
#define TIMER_ADC_MULTIPLIER 1

static inline void initialize_timer(void) {
    // ustaw tryb licznika
    // COM1A = 10   -- non-inverting mode
    // WGM1  = 1000 -- Phase and Frequency Correct PWM top=ICR1
    // CS1   = 010  -- prescaler 8
    // ICR1  = 8*1023
    // częstotliwość 16e6/(2*8*(1023)) = 977 Hz

    ICR1 = ADC_MAXIMUM * TIMER_ADC_MULTIPLIER;
    TCCR1A = _BV(COM1A1);
    TCCR1B = _BV(WGM13) | _BV(CS11);
    TIMSK1 = _BV(ICIE1) /* | _BV(TOIE1) */;
    // ustaw pin OC1A (PB1) jako wyjście
    DDRB |= _BV(PB1);
}

#define MOSFET_OPEN 0
#define MOSFET_CLOSE 1
#define ADC_OPEN_READY 2
#define ADC_CLOSE_READY 3
#define ADC_BUSY 4

static volatile uint8_t measure_register = 0;
static volatile uint16_t mosfet_close_adc = 0;

#define set_bit(reg, index) (reg) |= _BV(index);
#define clear_bit(reg, index) (reg) &= ~_BV(index);
#define get_bit(reg, index) ((reg)&_BV(index))

#define ENABLE_ADC_INTERRUPT set_bit(ADCSRA, ADIE);
#define DISABLE_ADC_INTERRUPT clear_bit(ADCSRA, ADIE);
#define START_ADC_CONVERSION set_bit(ADCSRA, ADSC);

static struct PID_DATA pidData;
static volatile int16_t goal_adc = 0;

// Timer/Counter1 Capture Event
ISR(TIMER1_CAPT_vect) {
    if (!get_bit(measure_register, ADC_BUSY) && !get_bit(measure_register, ADC_CLOSE_READY)/*  && get_bit(ADCSRA, ADIE) */) {
        set_bit(measure_register, MOSFET_CLOSE);
        set_bit(measure_register, ADC_BUSY);
        START_ADC_CONVERSION;
    }
}

ISR(ADC_vect) {
    /* if (get_bit(measure_register, MOSFET_OPEN)) {
        mosfet_open_adc = ADC;
        set_bit(measure_register, ADC_OPEN_READY);
        clear_bit(measure_register, MOSFET_OPEN);
        clear_bit(measure_register, ADC_BUSY);
    } else */
    if (get_bit(measure_register, MOSFET_CLOSE)) {
        mosfet_close_adc = ADC;
        set_bit(measure_register, ADC_CLOSE_READY);
        clear_bit(measure_register, MOSFET_CLOSE);
        clear_bit(measure_register, ADC_BUSY);
        int16_t pid = pid_Controller(goal_adc, mosfet_close_adc, &pidData);
        OCR1A = (pid * TIMER_ADC_MULTIPLIER) + 512;
    }
}

#define K_P 0.65 * 1.00
#define K_I 0.5 * 0.00
#define K_D 0.12 * 0.00

static inline void initialize_pid(void) {
    pid_Init(K_P * SCALING_FACTOR, K_I * SCALING_FACTOR, K_D * SCALING_FACTOR, &pidData);
}

int main(void) {
    // zainicjalizuj UART
    uart_init();
    // skonfiguruj strumienie wejścia/wyjścia
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    initialize_pid();
    initialize_adc();
    initialize_timer();

    sei();

    while (1) {
        cli();
        // DISABLE_ADC_INTERRUPT;

        if (get_bit(measure_register, ADC_CLOSE_READY)) {
            clear_bit(measure_register, ADC_CLOSE_READY);
            uint16_t adc_milivolts = adc_to_milivolts(mosfet_close_adc);
            printf("MOSFET close: %" PRIu16 "mV (PID: %" PRId16 ")\r\n", adc_milivolts, (int16_t)OCR1A - 512);
        }

        ADMUX &= ~MOTOR_MUX;
        ADMUX |= POTENTIOMETER_MUX;
        uint16_t adc = read_adc();
        goal_adc = adc;
        // OCR1A = adc * TIMER_ADC_MULTIPLIER;
        ADMUX &= ~POTENTIOMETER_MUX;
        ADMUX |= MOTOR_MUX;

        set_bit(TIFR1, ICF1);
        set_bit(TIFR1, TOV1);
        set_bit(ADCSRA, ADIF);
        sei();
        // ENABLE_ADC_INTERRUPT
        _delay_ms(100);
    }
}
