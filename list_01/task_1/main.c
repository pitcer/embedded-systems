#include <avr/io.h>
#include <inttypes.h>
#include <stdio.h>

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
    while (!(UCSR0A & _BV(UDRE0))) { }
    UDR0 = data;
    return 0;
}

// odczyt jednego znaku
int uart_receive(FILE* stream) {
    // czekaj aż znak dostępny
    while (!(UCSR0A & _BV(RXC0))) { }
    return UDR0;
}

#define PRINT_OPERATION(print_format, operation_result, operation_string)                      \
    printf("%" print_format " " operation_string " %" print_format " = %" print_format "\r\n", \
        first, second, operation_result)

#define HANDLE_TYPE(type, scan_format, print_format)          \
    printf("Podaj liczby oddzielone spacją...\r\n");          \
    type first;                                               \
    type second;                                              \
    scanf("%" scan_format " %" scan_format, &first, &second); \
    PRINT_OPERATION(print_format, first + second, "+");       \
    PRINT_OPERATION(print_format, first* second, "*");        \
    PRINT_OPERATION(print_format, first / second, "/");

void handle_int8() {
    HANDLE_TYPE(int8_t, SCNd8, PRId8);
}

void handle_int16() {
    HANDLE_TYPE(int16_t, SCNd16, PRId16);
}

void handle_int32() {
    HANDLE_TYPE(int32_t, SCNd32, PRId32);
}

void handle_int64() {
    printf("Podaj liczby oddzielone spacją...\r\n");
    int32_t first;
    int32_t second;
    scanf("%" SCNd32 " %" SCNd32, &first, &second);
    int64_t first_i64 = (int64_t)first;
    int64_t second_i64 = (int64_t)second;
    int64_t sum_i64 = first_i64 + second_i64;
    int64_t product_i64 = first_i64 * second_i64;
    int64_t quotient_i64 = first_i64 / second_i64;
    first = first_i64;
    second = second_i64;
    int32_t sum = sum_i64;
    int32_t product = product_i64;
    int32_t quotient = quotient_i64;
    PRINT_OPERATION(PRId32, sum, "+");
    PRINT_OPERATION(PRId32, product, "*");
    PRINT_OPERATION(PRId32, quotient, "/");
}

void handle_float() {
    HANDLE_TYPE(float, "f", "f");
}

void handle_type(uint8_t type) {
    switch (type) {
    case 1: {
        handle_int8();
    } break;
    case 2: {
        handle_int16();
    } break;
    case 3: {
        handle_int32();
    } break;
    case 4: {
        handle_int64(); // AVR nie wspiera SCNd64 i PRId64
    } break;
    case 5: {
        handle_float(); // trzeba dodać `-u vfscanf -lscanf_flt -u vfprintf -lprintf_flt` do LIBS
    } break;
    default: {
        printf("Nieprawidłowy typ danych.\r\n");
    } break;
    }
}

FILE uart_file;

int main() {
    // zainicjalizuj UART
    uart_init();
    // skonfiguruj strumienie wejścia/wyjścia
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    while (1) {
        printf("Wybierz typ danych:\r\n");
        printf("* 1 -> int8_t\r\n");
        printf("* 2 -> int16_t\r\n");
        printf("* 3 -> int32_t\r\n");
        printf("* 4 -> int64_t\r\n");
        printf("* 5 -> float\r\n");

        uint8_t type;
        scanf("%" SCNu8, &type);
        handle_type(type);
    }
}
