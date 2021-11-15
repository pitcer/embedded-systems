#include <avr/io.h>
#include <inttypes.h>
#include <stdio.h>
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

void initialize_timer() {
    // ustaw tryb licznika
    // WGM1  = 0000 -- normal
    // CS1   = 001  -- prescaler 1
    TCCR1B = _BV(CS10);
}

FILE uart_file;

uint16_t measure_empty(void);

#define DECLARE_EMPTY_ASSIGNMENT(type) \
    uint16_t measure_empty_assignment_##type(void);

#define DECLARE_OPERATION_FUNCTION(name, type) \
    uint16_t measure_##name##_##type(type, type);

#define DECLARE_OPERATIONS_FUNCTIONS(type) \
    DECLARE_OPERATION_FUNCTION(add, type); \
    DECLARE_OPERATION_FUNCTION(mul, type); \
    DECLARE_OPERATION_FUNCTION(div, type);

DECLARE_EMPTY_ASSIGNMENT(int8_t);
DECLARE_EMPTY_ASSIGNMENT(int16_t);
DECLARE_EMPTY_ASSIGNMENT(int32_t);
DECLARE_EMPTY_ASSIGNMENT(int64_t);
DECLARE_EMPTY_ASSIGNMENT(float);

DECLARE_OPERATIONS_FUNCTIONS(int8_t);
DECLARE_OPERATIONS_FUNCTIONS(int16_t);
DECLARE_OPERATIONS_FUNCTIONS(int32_t);
DECLARE_OPERATIONS_FUNCTIONS(int64_t);
DECLARE_OPERATIONS_FUNCTIONS(float);

#define PRINT_OPERATION_MEASUREMENT(measurement_name, operation, type, left, right)         \
    uint16_t measurement_name##_empty_measurement = measure_empty();                        \
    uint16_t measurement_name##_measurement = measurement_name(left, right);                \
    uint16_t measurement_name##_assignment_measurement = measure_empty_assignment_##type(); \
    printf(#operation " " #type ": %" PRIu16 " cycles\r\n", measurement_name##_measurement - measurement_name##_empty_measurement /* - (measurement_name##_assignment_measurement - measurement_name##_empty_measurement) */);

#define PRINT_MEASUREMENTS(type, left, right)                              \
    PRINT_OPERATION_MEASUREMENT(measure_add_##type, +, type, left, right); \
    PRINT_OPERATION_MEASUREMENT(measure_mul_##type, *, type, left, right); \
    PRINT_OPERATION_MEASUREMENT(measure_div_##type, /, type, left, right);

int main() {
    // zainicjalizuj UART
    uart_init();
    // skonfiguruj strumienie wejścia/wyjścia
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;
    // zainicjalizuj licznik
    initialize_timer();
    // program testowy
    while (1) {
        PRINT_MEASUREMENTS(int8_t, 123, 45);
        PRINT_MEASUREMENTS(int16_t, 123, 45);
        PRINT_MEASUREMENTS(int32_t, 123, 45);
        PRINT_MEASUREMENTS(int64_t, 123, 45);
        PRINT_MEASUREMENTS(float, 123, 45);
        _delay_ms(1000);
    }
}
