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
    // pomiar ADC3
    ADMUX |= ACTIVE_THERMISTOR_MUX;
    // wyłącz wejścia cyfrowe na ADC0
    DIDR0 = _BV(ACTIVE_THERMISTOR_MUX);
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

static inline uint32_t adc_to_milivolts(uint16_t adc) {
    const uint32_t vref_times_1000 = 1100;
    return (adc * vref_times_1000) / 1024;
}

static inline int32_t milivolts_to_decycelsius(uint32_t milivolts) {
    // const int32_t temperature_coefficient = 10;
    const int32_t zero_degree_volatage = 500;
    return (milivolts - zero_degree_volatage) /* * (10 / temperature_coefficient) */;
}

static inline uint32_t decycelsius_to_milivolts(int32_t decycelsius) {
    return decycelsius + 500;
}

static inline uint16_t milivolts_to_adc(uint32_t milivolts) {
    return (1024 * milivolts) / 1100;
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

/*! \brief P, I and D parameter values
 *
 * The K_P, K_I and K_D values (P, I and D gains)
 * need to be modified to adapt to the application at hand
 */
#define K_P 1.00
#define K_I 0.00
#define K_D 0.00

/*! \brief Flags for status information
 */
// struct GLOBAL_FLAGS {
//     //! True when PID control loop should run one time
//     uint8_t pidTimer : 1;
//     uint8_t dummy : 7;
// } gFlags = { 0, 0 };

//! Parameters for regulator
static struct PID_DATA pidData;

/*! \brief Sampling Time Interval
 *
 * Specify the desired PID sample time interval
 * With a 8-bit counter (255 cylces to overflow), the time interval value is calculated as follows:
 * TIME_INTERVAL = ( desired interval [sec] ) * ( frequency [Hz] ) / 255
 */
//! \xrefitem todo "Todo" "Todo list"
#define TIME_INTERVAL 157

/*! \brief Timer interrupt to control the sampling interval
 */
ISR(TIMER0_OVF_vect) {
    static uint16_t i = 0;
    if (i < TIME_INTERVAL) {
        i++;
    } else {
        i = 0;
        set_bit(flags, PID_TIMER_FLAG);
    }
}

static inline void initialize_pid(void) {
    pid_Init(K_P * SCALING_FACTOR, K_I * SCALING_FACTOR, K_D * SCALING_FACTOR, &pidData);

    // Set up timer, enable timer/counte 0 overflow interrupt
    TCCR0A = _BV(CS00);
    TIMSK0 = _BV(TOIE0);
    TCNT0 = 0;
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

    initialize_adc();
    sei();

    const int32_t temperature_delta = 10;

    while (1) {
        UCSR0B &= ~_BV(RXCIE0);
        printf("\r\nPodaj temperaturę [d°C]...\r\n");

        int32_t goal_temperature;
        scanf("%" SCNi32, &goal_temperature);
        const uint32_t goal_voltage = decycelsius_to_milivolts(goal_temperature);
        const uint16_t goal_adc = milivolts_to_adc(goal_voltage);

        const int32_t minimum_temperature = goal_temperature - temperature_delta;
        const uint32_t minimum_voltage = decycelsius_to_milivolts(minimum_temperature);
        const uint16_t minimum_adc = milivolts_to_adc(minimum_voltage);

        set_bit(flags, SAMPLING_ENABLED_FLAG);
        UCSR0B |= _BV(RXCIE0);

        enable_mosfet();
        while (get_bit(flags, SAMPLING_ENABLED_FLAG)) {
            const uint16_t adc = read_adc();
            const uint32_t volatage = adc_to_milivolts(adc);
            const int32_t temperature = milivolts_to_decycelsius(volatage);

            printf(
                "Temperatura: %" PRIu32 " [d°C] (%" PRIu32 " [mV], %" PRIu16 ")\r\n",
                temperature, volatage, adc);

            if (adc >= goal_adc) {
                disable_mosfet();
            } else if (adc <= minimum_adc) {
                enable_mosfet();
            }

            // Run PID calculations once every PID timer timeout
            if (get_bit(flags, PID_TIMER_FLAG)) {
                const int16_t input_value = pid_Controller(goal_adc, adc, &pidData);
                printf("PID: %" PRId16 "\r\n", input_value);
                clear_bit(flags, PID_TIMER_FLAG);
            }

            _delay_ms(500);
        }
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
