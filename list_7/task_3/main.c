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

#define BUFFER_SIZE 20

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

#define PARSE_ARGUMENT(error, then) \
    if (length > 1) {               \
        input++;                    \
        length--;                   \
        then;                       \
    } else {                        \
        error;                      \
    }

#define PARSE_DECIMAL_ARGUMENT(separator, argument_type, argument_name, error, then) \
    if (length > 1 && (*input == separator)) {                                       \
        input++;                                                                     \
        length--;                                                                    \
        char* previous_input = input;                                                \
        argument_type argument_name = strtol(input, &input, 10);                     \
        length = length - (input - previous_input);                                  \
        then;                                                                        \
    } else {                                                                         \
        error;                                                                       \
    }

#define i2cCheck(code, msg)                                   \
    if ((TWSR & 0xf8) != (code)) {                            \
        printf(msg " failed, status: %.2x\r\n", TWSR & 0xf8); \
        i2cReset();                                           \
        continue;                                             \
    }

#define RTC_ADDRESS 0xd0

#define SECONDS_REGISTER 0x0
#define MINUTES_REGISTER 0x1
#define HOURS_REGISTER 0x2
#define DATE_REGISTER 0x4
#define MONTH_CENTURY_REGISTER 0x5
#define YEAR_REGISTER 0x6

static uint8_t read_rtc(uint8_t address) {
    while (1) {
        i2cStart();
        i2cCheck(0x08, "I2C start");
        i2cSend(RTC_ADDRESS | 0x0);
        i2cCheck(0x18, "I2C RTC write request");
        i2cSend(address);
        i2cCheck(0x28, "I2C RTC set address");
        i2cStart();
        i2cCheck(0x10, "I2C second start");
        i2cSend(RTC_ADDRESS | 0x1);
        i2cCheck(0x40, "I2C RTC read request");
        uint8_t data = i2cReadNoAck();
        i2cCheck(0x58, "I2C RTC read");
        i2cStop();
        i2cCheck(0xf8, "I2C stop");
        return data;
    }
}

#define FROM_BCD(binary) \
    (((binary & 0xF0) >> 4) * 10 + (binary & 0x0F))

static inline void handle_date(void) {
    const uint8_t date = read_rtc(DATE_REGISTER);
    const uint8_t month = read_rtc(MONTH_CENTURY_REGISTER);
    uint16_t century = month & 0x80 ? 2000 : 1900;
    const uint8_t year = read_rtc(YEAR_REGISTER);
    printf(
        "%.2u-%.2u-%.4u\r\n",
        FROM_BCD(date & 0x3F),
        FROM_BCD(month & 0x1F),
        century + FROM_BCD(year));
}

static inline void handle_time(void) {
    const uint8_t seconds = read_rtc(SECONDS_REGISTER);
    const uint8_t minutes = read_rtc(MINUTES_REGISTER);
    const uint8_t hours = read_rtc(HOURS_REGISTER);
    printf(
        "%.2u:%.2u:%.2u\r\n",
        FROM_BCD(hours & 0x3F),
        FROM_BCD(minutes & 0x7F),
        FROM_BCD(seconds & 0x7F));
}

static void write_rtc(uint8_t address, uint8_t data) {
    while (1) {
        i2cStart();
        i2cCheck(0x08, "I2C start");
        i2cSend(RTC_ADDRESS | 0x0);
        i2cCheck(0x18, "I2C RTC write request");
        i2cSend(address);
        i2cCheck(0x28, "I2C RTC set address");
        i2cSend(data);
        i2cCheck(0x28, "I2C RTC write");
        i2cStop();
        i2cCheck(0xf8, "I2C stop");
        return;
    }
}

#define INTO_BCD(decimal) \
    ((decimal / 10) << 4 | (decimal % 10))

static inline void handle_set_date(uint8_t day, uint8_t month, uint16_t year) {
    write_rtc(DATE_REGISTER, INTO_BCD(day));
    const uint8_t century = (year >= 2000) << 7;
    write_rtc(MONTH_CENTURY_REGISTER, century | INTO_BCD(month));
    write_rtc(YEAR_REGISTER, INTO_BCD(year % 100));
}

static inline void handle_set_time(uint8_t hours, uint8_t minutes, uint8_t seconds) {
    write_rtc(HOURS_REGISTER, INTO_BCD(hours));
    write_rtc(MINUTES_REGISTER, INTO_BCD(minutes));
    write_rtc(SECONDS_REGISTER, INTO_BCD(seconds));
}

int main(void) {
    // zainicjalizuj UART
    uart_init();
    // skonfiguruj strumienie wejścia/wyjścia
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    // zainicjalizuj I2C
    i2cInit();

    printf("Commands: date|time|set [date|time] [DD-MM-YYYY|HH:MM:SS]\r\n");

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

        // clang-format off
        PARSE_COMMAND (
            "date", 4,
            handle_date()
        ) else PARSE_COMMAND (
            "time", 4,
            handle_time()
        ) else PARSE_COMMAND (
            "set", 3,
            PARSE_ARGUMENT (
                printf("set: invalid subcommand\r\n"),
                PARSE_COMMAND (
                    "date", 4,
                    PARSE_DECIMAL_ARGUMENT (
                        ' ', uint8_t, day,
                        printf("set date: invalid day argument\r\n"),
                    PARSE_DECIMAL_ARGUMENT (
                        '-', uint8_t, month,
                        printf("set date: invalid month argument\r\n"),
                    PARSE_DECIMAL_ARGUMENT (
                        '-', uint16_t, year,
                        printf("set date: invalid year argument\r\n"),
                        handle_set_date(day, month, year)
                    )))
                ) else PARSE_COMMAND (
                    "time", 4,
                    PARSE_DECIMAL_ARGUMENT(
                        ' ', uint8_t, hours,
                        printf("set time: invalid hours argument\r\n"),
                    PARSE_DECIMAL_ARGUMENT(
                        ':', uint8_t, minutes,
                        printf("set time: invalid minutes argument\r\n"),
                    PARSE_DECIMAL_ARGUMENT(
                        ':', uint8_t, seconds,
                        printf("set time: invalid seconds argument\r\n"),
                        handle_set_time(hours, minutes, seconds)
                    )))
                ) else {
                    printf("set: invalid subcommand: %s\r\n", input);
                }
            )
        ) else {
            printf("Invalid command: %s\r\n", input);
        }
        // clang-format on
    }
}
