#include "hd44780.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <util/delay.h>

#define ERROR_LED PB5
#define ERROR_LED_DDR DDRB
#define ERROR_LED_PORT PORTB

#define BAUD 9600 // baudrate
#define UBRR_VALUE ((F_CPU) / 16 / (BAUD)-1) // zgodnie ze wzorem

#define LCD_WIDTH 16

#define BUFFER_SIZE LCD_WIDTH
#define BUFFER_DATA uint8_t
#define BUFFER_POINTER uint8_t

#define BUFFER_INITIALIZE(name)                                 \
    static void initialize_##name(BUFFER_DATA initial_value) {  \
        name##_read_pointer = 0;                                \
        name##_write_pointer = 0;                               \
        for (uint8_t index = 0; index < BUFFER_SIZE; index++) { \
            name##_buffer[index] = initial_value;               \
        }                                                       \
    }

#define BUFFER_READ(name)                               \
    static BUFFER_DATA name##_read(void) {              \
        BUFFER_POINTER read = name##_read_pointer;      \
        name##_read_pointer = (read + 1) % BUFFER_SIZE; \
        return name##_buffer[read];                     \
    }

#define BUFFER_WRITE(name)                                \
    static void name##_write(BUFFER_DATA data) {          \
        BUFFER_POINTER write = name##_write_pointer;      \
        name##_write_pointer = (write + 1) % BUFFER_SIZE; \
        name##_buffer[write] = data;                      \
    }

#define BUFFER_IS_EMPTY(name)                               \
    static bool is_##name##_empty(void) {                   \
        return name##_read_pointer == name##_write_pointer; \
    }

#define BUFFER_CREATE(name)                                 \
    static volatile BUFFER_DATA name##_buffer[BUFFER_SIZE]; \
    static volatile BUFFER_POINTER name##_read_pointer;     \
    static volatile BUFFER_POINTER name##_write_pointer;    \
    BUFFER_INITIALIZE(name);                                \
    BUFFER_READ(name);                                      \
    BUFFER_WRITE(name);                                     \
    BUFFER_IS_EMPTY(name);

BUFFER_CREATE(line);

static void initialize_uart(void) {
    // ustaw baudrate
    UBRR0 = UBRR_VALUE;
    // wyczyść USART Data Register Empty
    UCSR0A &= ~_BV(UDRE0);
    // receive complete interrupt enable
    UCSR0B |= _BV(RXCIE0);
    // enable receiver
    UCSR0B |= _BV(RXEN0);
    UCSR0C = _BV(UCSZ00) | _BV(UCSZ01); // character size 8 bits; set by default
}

static volatile uint8_t receiver_register;

#define is_bit_set(position) receiver_register& _BV((position))
#define set_bit(position) receiver_register |= _BV((position))
#define clear_bit(position) receiver_register &= ~_BV((position))

#define NEW_CHARACTER_RECEIVED 0
#define NEW_LINE_RECEIVED 1

// Receive Complete Interrupt
ISR(USART_RX_vect) {
    const uint8_t input = UDR0;
    switch (input) {
    case '\r': {
        break;
    }
    case '\n': {
        set_bit(NEW_LINE_RECEIVED);
        break;
    }
    default: {
        line_write(input);
        set_bit(NEW_CHARACTER_RECEIVED);
        break;
    }
    }
}

static int uart_receive(FILE* stream) {
    if (is_line_empty()) {
        // Ta sytuacja nigdy nie powinna się wydarzyć chyba, że napisaliśmy źle program.
        // Przed wywołaniem powinniśmy sprawdzić czy NEW_CHARACTER_RECEIVED jest ustawione.
        ERROR_LED_PORT |= _BV(ERROR_LED);
        return 0;
    }

    BUFFER_DATA result = line_read();
    return result;
}

static int hd44780_transmit(char data, FILE* stream) {
    LCD_WriteData(data);
    return 0;
}

static FILE io_file;

#define TICK_RATE 1000
#define TICK_PERIOD 1

// static inline void set_cursor() {
//     LCD_GoTo(line_write_pointer, 1);
//     putchar('_');
// }

// static inline void clear_cursor() {
//     LCD_GoTo(line_write_pointer, 1);
//     putchar(' ');
// }

// static inline void blink_cursor(uint16_t tick) {
//     if (tick == 0) {
//         set_cursor();
//     } else if (tick == TICK_RATE / 2) {
//         clear_cursor();
//     }
// }

static inline void print_buffer(void) {
    for (uint8_t index = 0; index < BUFFER_SIZE; index++) {
        char character = line_buffer[index];
        putchar(character);
    }
}

static inline void dump_buffer_with_echo(void) {
    while (!is_line_empty()) {
        LCD_GoTo(line_read_pointer, 1);
        char character = getchar();
        putchar(character);
    }
}

static inline void clear_line() {
    for (uint8_t index = 0; index < LCD_WIDTH; index++) {
        putchar(' ');
    }
}

static inline void enable_cursor(void) {
    LCD_WriteCommand(HD44780_DISPLAY_ONOFF | HD44780_DISPLAY_ON | HD44780_CURSOR_ON | HD44780_CURSOR_BLINK);
}

static inline void disable_cursor(void) {
    LCD_WriteCommand(HD44780_DISPLAY_ONOFF | HD44780_DISPLAY_ON | HD44780_CURSOR_ON | HD44780_CURSOR_BLINK);
}

int main(void) {
    ERROR_LED_DDR |= _BV(ERROR_LED);

    LCD_Initialize();
    LCD_Clear();

    initialize_line(' ');
    initialize_uart();

    fdev_setup_stream(&io_file, hd44780_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &io_file;

    LCD_GoTo(0, 1);
    enable_cursor();

    uint16_t tick_counter = 0;
    while (1) {
        cli();
        if (is_bit_set(NEW_CHARACTER_RECEIVED)) {
            disable_cursor();
            dump_buffer_with_echo();

            LCD_GoTo(line_read_pointer, 1);
            enable_cursor();

            clear_bit(NEW_CHARACTER_RECEIVED);
            tick_counter = 0;
        } else if (is_bit_set(NEW_LINE_RECEIVED)) {
            disable_cursor();
            LCD_GoTo(0, 0);
            print_buffer();

            initialize_line(' ');

            LCD_GoTo(0, 1);
            clear_line();

            LCD_GoTo(0, 1);
            enable_cursor();

            clear_bit(NEW_LINE_RECEIVED);
            tick_counter = 0;
        }
        sei();

        // blink_cursor(tick_counter);

        _delay_ms(TICK_PERIOD);
        tick_counter++;
        if (tick_counter == TICK_RATE + 1) {
            tick_counter = 0;
        }
    }
}
