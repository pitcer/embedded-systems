#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>

#define MOSFET PB5
#define MOSFET_DDR DDRB
#define MOSFET_PORT PORTB

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

#define ACTIVE_THERMISTOR_MUX ADC3D

static inline void initialize_adc(void) {
    ADMUX = _BV(REFS1) | _BV(REFS0); // referencja AVcc = 1.1V
    // pomiar ADC3
    ADMUX |= ACTIVE_THERMISTOR_MUX;
    // wyłącz wejścia cyfrowe na ADC3
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

static inline float read_voltage(void) {
    uint16_t adc = read_adc();
    const float vref = 1.1;
    return (adc * vref) / 1024.0;
}

static inline float voltage_to_temperature(float voltage) {
    const float temperature_coefficient = 10.0 / 1000.0;
    const float zero_degree_volatage = 500.0 / 1000.0;
    return (voltage - zero_degree_volatage) / temperature_coefficient;
}

static volatile uint8_t sampling_enabled = 1;

// Receive Complete Interrupt
ISR(USART_RX_vect) {
    const uint8_t input = UDR0;
    if (input == 's') {
        sampling_enabled = 0;
    }
}

#define ENABLE_MOSFET MOSFET_PORT |= _BV(MOSFET);
#define DISABLE_MOSFET MOSFET_PORT &= ~_BV(MOSFET);

int main(void) {
    MOSFET_DDR |= _BV(MOSFET);
    DISABLE_MOSFET;

    // zainicjalizuj UART
    uart_init();
    // skonfiguruj strumienie wejścia/wyjścia
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    initialize_adc();
    sei();

    const uint8_t temperature_delta = 1;
    float goal_temperature = 0;
    while (1) {
        UCSR0B &= ~_BV(RXCIE0);
        printf("\r\nPodaj temperaturę...\r\n");
        scanf("%f", &goal_temperature);
        sampling_enabled = 1;
        UCSR0B |= _BV(RXCIE0);

        ENABLE_MOSFET;
        while (sampling_enabled) {
            float voltage = read_voltage();
            float temperature = voltage_to_temperature(voltage);

            printf("Termperatura: %f°C (%f V)\r\n", temperature, voltage);

            if (temperature >= goal_temperature) {
                DISABLE_MOSFET;
            } else if (temperature <= goal_temperature - temperature_delta) {
                ENABLE_MOSFET;
            }

            _delay_ms(1000);
        }
        DISABLE_MOSFET;
    }
}
