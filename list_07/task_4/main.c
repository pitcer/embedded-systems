#include <avr/interrupt.h>
#include <avr/io.h>
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

FILE uart_file;

#define MASTER_PORT PORTD
#define MASTER_PIN PIND
#define MASTER_MISO PD6
#define MASTER_MOSI PD5
#define MASTER_SS PD4
#define MASTER_SCK PD7

// inicjalizacja SPI
static void initialize_spi() {
    // ustaw piny MISO, M_MOSI, M_SCK, M_SS jako wyjścia
    DDRB |= _BV(PB4);
    DDRD |= _BV(MASTER_MOSI) | _BV(MASTER_SCK) | _BV(MASTER_SS);
    // włącz SPI w trybie slave
    SPCR = _BV(SPE);
}

#define SET(port, pin) (port) |= _BV(pin)
#define CLEAR(port, pin) (port) &= ~_BV(pin)
#define SET_VALUE(port, pin, value) (port) = (((port) & ~_BV(pin)) | ((value) << (pin)))
#define GET(port, pin) ((port >> pin) & 0x1)

#define MASTER_SET(pin) SET(MASTER_PORT, pin)
#define MASTER_CLEAR(pin) CLEAR(MASTER_PORT, pin)
#define MASTER_SET_VALUE(pin, value) SET_VALUE(MASTER_PORT, pin, value)
#define MASTER_GET(pin) GET(MASTER_PIN, pin)

static uint8_t spi_slave_receive(void) {
    return SPDR;
}

static void spi_slave_transmit(uint8_t data) {
    SPDR = data;
}

static void spi_master_transmit(uint8_t data) {
    MASTER_CLEAR(MASTER_SS);
    MASTER_CLEAR(MASTER_SCK);

    for (uint8_t index = 0; index < 8; index++) {
        const uint8_t bit = (data >> (7 - index)) & 0x01;
        MASTER_SET_VALUE(MASTER_MOSI, bit);
        _delay_us(1);
        MASTER_SET(MASTER_SCK);
        _delay_us(1);
        MASTER_CLEAR(MASTER_MOSI);
        MASTER_CLEAR(MASTER_SCK);
    }

    MASTER_SET(MASTER_MOSI);
    MASTER_CLEAR(MASTER_SCK);
    MASTER_SET(MASTER_SS);
}

static uint8_t spi_master_receive(void) {
    uint8_t result = 0;

    MASTER_CLEAR(MASTER_SS);
    MASTER_CLEAR(MASTER_SCK);

    for (uint8_t index = 0; index < 8; index++) {
        _delay_us(1);
        MASTER_SET(MASTER_SCK);
        uint8_t bit = MASTER_GET(MASTER_MISO);
        result |= bit << (7 - index);
        _delay_us(1);
        MASTER_CLEAR(MASTER_SCK);
    }

    MASTER_CLEAR(MASTER_SCK);
    MASTER_SET(MASTER_SS);

    return result;
}

int main(void) {
    // zainicjalizuj UART
    uart_init();
    // skonfiguruj strumienie wejścia/wyjścia
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    MASTER_SET(MASTER_SS);
    MASTER_SET(MASTER_MOSI);
    SET(PORTB, PB4);

    initialize_spi();

    uint8_t master_data = 0;
    while (1) {
        spi_master_transmit(master_data);
        printf("[master] transmitted: %" PRIx8 "\r\n", master_data);
        _delay_ms(250);

        uint8_t slave_data = spi_slave_receive();
        printf("[slave] received: %" PRIx8 "\r\n", slave_data);
        _delay_ms(250);

        slave_data++;
        spi_slave_transmit(slave_data);
        printf("[slave] transmitted: %" PRIx8 "\r\n", slave_data);
        _delay_ms(250);

        master_data = spi_master_receive();
        printf("[master] received: %" PRIx8 "\r\n", master_data);
        _delay_ms(250);

        master_data++;
    }
}
