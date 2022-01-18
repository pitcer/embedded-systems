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
    // receive complete interrupt enable
    UCSR0B |= _BV(RXCIE0);
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

#define ACTIVE_THERMISTOR_MUX ADC0D

static inline void initialize_adc(void) {
    ADMUX = _BV(REFS1) | _BV(REFS0); // referencja AVcc = 1.1V
    // pomiar ADC0
    ADMUX |= ACTIVE_THERMISTOR_MUX;
    // wyłącz wejścia cyfrowe na ADC0
    DIDR0 = _BV(ACTIVE_THERMISTOR_MUX);
    // częstotliwość zegara ADC 125 kHz (16 MHz / 128)
    ADCSRA = _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); // preskaler 128
    ADCSRA |= _BV(ADIE) | _BV(ADATE); // enable interrupt and auto-trigger
    ADCSRB |= _BV(ADTS2) | _BV(ADTS1) | _BV(ADTS0); // auto-trigger on timer 1 input capture
    // ADCSRA |= _BV(ADSC); // wykonaj konwersję
    ADCSRA |= _BV(ADEN); // włącz ADC
}

static inline uint16_t read_adc(void) {
    ADCSRA |= _BV(ADSC); // wykonaj konwersję
    loop_until_bit_is_set(ADCSRA, ADIF); // czekaj na wynik
    ADCSRA |= _BV(ADIF); // wyczyść bit ADIF (pisząc 1!)
    return ADC; // weź zmierzoną wartość (0..1023)
}

static inline uint16_t adc_to_milivolts(uint32_t adc) {
    const uint32_t vref_times_1000 = 1100;
    return (adc * vref_times_1000) / 1024;
}

static inline int16_t milivolts_to_decycelsius(uint16_t milivolts) {
    // const int32_t temperature_coefficient = 10;
    const int32_t zero_degree_volatage = 500;
    return (milivolts - zero_degree_volatage) /* * (10 / temperature_coefficient) */;
}

static inline uint16_t decycelsius_to_milivolts(int16_t decycelsius) {
    return decycelsius + 500;
}

static inline uint16_t milivolts_to_adc(uint32_t milivolts) {
    return (1024 * milivolts) / 1100;
}

static inline int16_t clamp(int16_t lower, int16_t upper, int16_t value) {
    if (value < lower) {
        return lower;
    } else if (value > upper) {
        return upper;
    } else {
        return value;
    }
}

#define set_bit(reg, index) (reg) |= _BV(index);
#define clear_bit(reg, index) (reg) &= ~_BV(index);
#define get_bit(reg, index) ((reg)&_BV(index))

static volatile uint8_t flags;
#define PID_TIMER_FLAG 0
#define SAMPLING_ENABLED_FLAG 1

// Receive Complete Interrupt
ISR(USART_RX_vect) {
    const uint8_t input = UDR0;
    if (input == 's') {
        clear_bit(flags, SAMPLING_ENABLED_FLAG);
    }
}

#define LED PB5
#define LED_DDR DDRB
#define LED_PORT PORTB

#define MOSFET PB4
#define MOSFET_DDR DDRB
#define MOSFET_PORT PORTB

static inline void enable_mosfet(void) {
    set_bit(MOSFET_PORT, MOSFET);
    set_bit(LED_PORT, LED);
}

static inline void disable_mosfet(void) {
    clear_bit(MOSFET_PORT, MOSFET);
    clear_bit(LED_PORT, LED);
}

// static inline void initialize_timer(void) {
//     // ustaw tryb licznika
//     // COM1A = 10   -- non-inverting mode
//     // WGM1  = 1110 -- fast PWM top=ICR1
//     // CS1   = 101  -- prescaler 1024
//     // ICR1  = 255
//     // częstotliwość 16e6/(1024*(1+255)) = 61 Hz
//     // wzór: datasheet 20.12.3 str. 164

//     ICR1 = 255;
//     TCCR1A = _BV(COM1A1) | _BV(WGM11);
//     TCCR1B = _BV(WGM12) | _BV(WGM13) | _BV(CS12) | _BV(CS10);
//     TIMSK1 = _BV(TOIE1); // enable overflow interrupt for ADC auto-trigger
//     // ustaw pin OC1A (PB1) jako wyjście
//     DDRB |= _BV(PB1);
//     OCR1A = 0;
// }

static inline void initialize_timer(void) {
    // ustaw tryb licznika
    // COM1A = 10   -- non-inverting mode
    // WGM1  = 1000 -- Phase and Frequency Correct PWM top=ICR1
    // CS1   = 101  -- prescaler 1024
    // ICR1  = 256
    // częstotliwość 16e6/(2*1024*256) = 30.5 Hz

    ICR1 = SCALING_FACTOR;
    TCCR1A = _BV(COM1A1);
    TCCR1B = _BV(WGM13) | _BV(CS12) | _BV(CS10);
    TIMSK1 = _BV(ICIE1) /* | _BV(TOIE1) */;
    // ustaw pin OC1A (PB1) jako wyjście
    DDRB |= _BV(PB1);
    OCR1A = 0;
}

static struct PID_DATA pidData;
static volatile int16_t goal = 0;
static volatile int16_t last_pid_input = 0;
static volatile uint16_t last_adc = 0;

// ISR(TIMER1_OVF_vect) {
//     const uint16_t shift = 0;
//     OCR1A = clamp(-SCALING_FACTOR / 2 + shift, SCALING_FACTOR / 2 + shift, last_pid_input) + SCALING_FACTOR / 2 - shift;
// }

ISR(TIMER1_CAPT_vect) { }

ISR(ADC_vect) {
    last_adc = ADC;
    last_pid_input = pid_Controller(goal, last_adc, &pidData);
    if (last_pid_input > 0) {
        enable_mosfet();
    } else {
        disable_mosfet();
    }
}

// ISR(TIMER0_OVF_vect) {
//     const uint16_t adc = read_adc();
//     const int16_t input_value = pid_Controller(goal, adc, &pidData);
//     if (input_value > 0) {
//         enable_mosfet();
//     } else {
//         disable_mosfet();
//     }
// }

/*! \brief P, I and D parameter values
 *
 * The K_P, K_I and K_D values (P, I and D gains)
 * need to be modified to adapt to the application at hand
 */
#define K_P 0.65 * 6.00
#define K_I 0.5 * 0.10
#define K_D 0.12 * 0.10

static inline void initialize_pid(void) {
    pid_Init(K_P * SCALING_FACTOR, K_I * SCALING_FACTOR, K_D * SCALING_FACTOR, &pidData);
}

int main(void) {
    set_bit(LED_DDR, LED);
    set_bit(MOSFET_DDR, MOSFET);
    disable_mosfet();

    // zainicjalizuj UART
    uart_init();
    // skonfiguruj strumienie wejścia/wyjścia
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    initialize_pid();
    initialize_timer();
    initialize_adc();

    while (1) {
        UCSR0B &= ~_BV(RXCIE0);
        printf("\r\nPodaj temperaturę [d°C]...\r\n");

        int16_t goal_temperature;
        scanf("%" SCNi16, &goal_temperature);
        const uint16_t goal_voltage = decycelsius_to_milivolts(goal_temperature);
        const uint16_t goal_adc = milivolts_to_adc(goal_voltage);
        goal = goal_adc;

        set_bit(flags, SAMPLING_ENABLED_FLAG);
        UCSR0B |= _BV(RXCIE0);

        enable_mosfet();
        sei();
        while (get_bit(flags, SAMPLING_ENABLED_FLAG)) {
            const uint16_t adc = last_adc;
            const uint16_t volatage = adc_to_milivolts(adc);
            const int16_t temperature = milivolts_to_decycelsius(volatage);

            printf(
                "Temperature: %" PRIi16 " [d°C] (%" PRIu16 " [mV], %" PRIu16 ") PID: %" PRId16 "\r\n",
                temperature, volatage, adc, last_pid_input);

            _delay_ms(100);
        }
        cli();
        OCR1A = 0;
        disable_mosfet();
    }
}

/*! \mainpage
 * \section Intro Introduction
 * This documents data structures, functions, variables, defines, enums, and
 * typedefs in the software for application note AVR221.
 *
 * \section CI Compilation Info
 * This software was written for the IAR Embedded Workbench 4.11A.
 *
 * To make project:
 * <ol>
 * <li> Add the file main.c and pid.c to project.
 * <li> Under processor configuration, select desired Atmel AVR device.
 * <li> Enable bit definitions in I/O include files
 * <li> High optimization on speed is recommended for best performance
 * </ol>
 *
 * \section DI Device Info
 * The included source code is written for all Atmel AVR devices.
 *
 * \section TDL ToDo List
 * \todo Put in own code in:
 * \ref Get_Reference(void), \ref Get_Measurement(void) and \ref Set_Input(int16_t inputValue)
 *
 * \todo Modify the \ref K_P (P), \ref K_I (I) and \ref K_D (D) gain to adapt to your application
 * \todo Specify the sampling interval time \ref TIME_INTERVAL
 */
