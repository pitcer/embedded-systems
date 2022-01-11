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

#define POTENTIOMETER_MUX ADC0D
#define MOTOR_MUX ADC1D

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

static inline uint32_t adc_to_milivolts(uint32_t adc) {
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
    TIMSK1 = _BV(ICIE1) | _BV(TOIE1);
    // ustaw pin OC1A (PB1) jako wyjście
    DDRB |= _BV(PB1);
}

#define MOSFET_OPEN 0
#define MOSFET_CLOSE 1
#define ADC_OPEN_READY 2
#define ADC_CLOSE_READY 3
#define ADC_BUSY 4

static volatile uint8_t measure_register = 0;
static volatile uint16_t mosfet_open_adc = 0;
static volatile uint16_t mosfet_close_adc = 0;

#define set_bit(reg, index) (reg) |= _BV(index);
#define clear_bit(reg, index) (reg) &= ~_BV(index);
#define get_bit(reg, index) ((reg)&_BV(index))

#define ENABLE_ADC_INTERRUPT set_bit(ADCSRA, ADIE);
#define DISABLE_ADC_INTERRUPT clear_bit(ADCSRA, ADIE);
#define START_ADC_CONVERSION set_bit(ADCSRA, ADSC);

// Timer/Counter1 Overflow
ISR(TIMER1_OVF_vect) {
    if (!get_bit(measure_register, ADC_BUSY) && !get_bit(measure_register, ADC_OPEN_READY)) {
        set_bit(measure_register, MOSFET_OPEN);
        set_bit(measure_register, ADC_BUSY);
        START_ADC_CONVERSION;
    }
}

// Timer/Counter1 Capture Event
ISR(TIMER1_CAPT_vect) {
    if (!get_bit(measure_register, ADC_BUSY) && !get_bit(measure_register, ADC_CLOSE_READY)) {
        set_bit(measure_register, MOSFET_CLOSE);
        set_bit(measure_register, ADC_BUSY);
        START_ADC_CONVERSION;
    }
}

ISR(ADC_vect) {
    if (get_bit(measure_register, MOSFET_OPEN)) {
        mosfet_open_adc = ADC;
        set_bit(measure_register, ADC_OPEN_READY);
        clear_bit(measure_register, MOSFET_OPEN);
        clear_bit(measure_register, ADC_BUSY);
    } else if (get_bit(measure_register, MOSFET_CLOSE)) {
        mosfet_close_adc = ADC;
        set_bit(measure_register, ADC_CLOSE_READY);
        clear_bit(measure_register, MOSFET_CLOSE);
        clear_bit(measure_register, ADC_BUSY);
    }
}

int main(void) {
    // zainicjalizuj UART
    uart_init();
    // skonfiguruj strumienie wejścia/wyjścia
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    initialize_adc();
    initialize_timer();

    sei();

    while (1) {
        cli();

        if (get_bit(measure_register, ADC_OPEN_READY)) {
            clear_bit(measure_register, ADC_OPEN_READY);
            uint32_t adc_milivolts = adc_to_milivolts(mosfet_open_adc);
            printf("MOSFET open: %" PRIu32 "mV\r\n", adc_milivolts);
        }
        if (get_bit(measure_register, ADC_CLOSE_READY)) {
            clear_bit(measure_register, ADC_CLOSE_READY);
            uint32_t adc_milivolts = adc_to_milivolts(mosfet_close_adc);
            printf("MOSFET close: %" PRIu32 "mV\r\n", adc_milivolts);
        }

        // DISABLE_ADC_INTERRUPT;
        ADMUX &= ~MOTOR_MUX;
        ADMUX |= POTENTIOMETER_MUX;
        uint16_t adc = read_adc();
        OCR1A = adc * TIMER_ADC_MULTIPLIER;
        ADMUX &= ~POTENTIOMETER_MUX;
        ADMUX |= MOTOR_MUX;
        // ENABLE_ADC_INTERRUPT

        set_bit(TIFR1, ICF1);
        set_bit(TIFR1, TOV1);
        set_bit(ADCSRA, ADIF);
        sei();

        _delay_ms(100);
    }
}
