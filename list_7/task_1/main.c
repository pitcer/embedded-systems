#include "i2c.h"
#include <avr/io.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

const uint8_t eeprom_addr = 0xa0;

#define i2cCheck(code, msg)                                   \
    if ((TWSR & 0xf8) != (code)) {                            \
        printf(msg " failed, status: %.2x\r\n", TWSR & 0xf8); \
        i2cReset();                                           \
        return;                                               \
    }

#define BUFFER_SIZE 16

static void read_input(char* buffer, uint8_t* length) {
    uint8_t index = 0;
    uint8_t character = getchar();
    while (character != '\r' && index < BUFFER_SIZE) {
        buffer[index] = character;
        character = getchar();
        index++;
    }
    if (character == '\r') {
        (void)getchar(); // also read \n character
    }
    *length = index;
    if (index < BUFFER_SIZE) {
        buffer[index] = '\0';
    }
}

#define PARSE_COMMAND(name, name_length, handler)                          \
    if (length >= name_length && strncmp(input, name, name_length) == 0) { \
        length -= name_length;                                             \
        input += name_length;                                              \
        handler;                                                           \
    }

#define PARSE_HEX_ARGUMENT(argument_type, argument_name, error_handler, then_handler) \
    if (length > 1) {                                                                 \
        input++;                                                                      \
        length--;                                                                     \
        char* previous_input = input;                                                 \
        argument_type argument_name = strtol(input, &input, 16);                      \
        length = length - (input - previous_input);                                   \
        then_handler;                                                                 \
    } else {                                                                          \
        error_handler;                                                                \
    }

static void handle_read(uint16_t address) {
    i2cStart();
    i2cCheck(0x08, "I2C start");
    i2cSend(eeprom_addr | ((address & 0x100) >> 7));
    i2cCheck(0x18, "I2C EEPROM write request");
    i2cSend(address & 0xff);
    i2cCheck(0x28, "I2C EEPROM set address");
    i2cStart();
    i2cCheck(0x10, "I2C second start");
    i2cSend(eeprom_addr | 0x1 | ((address & 0x100) >> 7));
    i2cCheck(0x40, "I2C EEPROM read request");
    uint8_t data = i2cReadNoAck();
    i2cCheck(0x58, "I2C EEPROM read");
    i2cStop();
    i2cCheck(0xf8, "I2C stop");
    printf("%.3x: %x\r\n", address, data);
}

static void handle_write(uint16_t address, uint8_t data) {
    i2cStart();
    i2cCheck(0x08, "I2C start");
    i2cSend(eeprom_addr | ((address & 0x100) >> 7));
    i2cCheck(0x18, "I2C EEPROM write request");
    i2cSend(address & 0xff);
    i2cCheck(0x28, "I2C EEPROM set address");
    i2cSend(data);
    i2cCheck(0x28, "I2C EEPROM write");
    i2cStop();
    i2cCheck(0xf8, "I2C stop");
    printf("%.3x <- %x\r\n", address, data);
}

int main(void) {
    // zainicjalizuj UART
    uart_init();
    // skonfiguruj strumienie wejścia/wyjścia
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    // zainicjalizuj I2C
    i2cInit();

    printf("Commands: read|write <address> [data]\r\n");

    while (1) {
        printf("> ");
        char buffer[BUFFER_SIZE] = { 0 };
        char* input = buffer;
        uint8_t length;
        read_input(input, &length);

        if (length == BUFFER_SIZE) {
            printf("\r\nCommand length exceed.\r\n");
            continue;
        }

        PARSE_COMMAND(
            "read", 4,
            PARSE_HEX_ARGUMENT(
                uint16_t, address,
                printf("read: invalid address\r\n"),
                handle_read(address & 0x1ff)))
        else PARSE_COMMAND(
            "write", 5,
            PARSE_HEX_ARGUMENT(
                uint16_t, address,
                printf("write: invalid address\r\n"),
            PARSE_HEX_ARGUMENT(
                uint8_t, data,
                printf("write: invalid data\r\n"),
                handle_write(address & 0x1ff, data)))
        ) else {
            printf("Invalid command: %s\r\n", input);
        }
    }
}
