#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>

#define BAUD 9600 // baudrate
#define UBRR_VALUE ((F_CPU) / 16 / (BAUD)-1) // zgodnie ze wzorem

// inicjalizacja UART
static void initialize_uart() {
    // ustaw baudrate
    UBRR0 = UBRR_VALUE;
    // wyczyść USART Data Register Empty
    UCSR0A &= ~_BV(UDRE0);
    UCSR0B |= _BV(RXCIE0); // receive complete interrupt enable
    UCSR0B |= _BV(TXCIE0); // transmit complete interrupt enable
    // UCSR0B |= _BV(UDRIE0); // Data Register Empty Interrupt Enable
    // włącz odbiornik i nadajnik
    UCSR0B |= _BV(RXEN0); // enable receiver
    UCSR0B |= _BV(TXEN0); // enable transmitter
    UCSR0C = _BV(UCSZ00) | _BV(UCSZ01); // character size 8 bits; set by default
}

#define BUFFER_SIZE 256
#define BUFFER_DATA uint8_t
#define BUFFER_POINTER uint8_t

#define BUFFER_CREATE(name)                                 \
    static volatile BUFFER_DATA name##_buffer[BUFFER_SIZE]; \
    static volatile BUFFER_POINTER name##_read = 0;         \
    static volatile BUFFER_POINTER name##_write = 0;

#define BUFFER_READ(name, result)               \
    {                                           \
        BUFFER_POINTER read = name##_read;      \
        name##_read = (read + 1) % BUFFER_SIZE; \
        result = name##_buffer[read];           \
    }

#define BUFFER_WRITE(name, data)                  \
    {                                             \
        BUFFER_POINTER write = name##_write;      \
        name##_write = (write + 1) % BUFFER_SIZE; \
        name##_buffer[write] = data;              \
    }

#define BUFFER_IS_EMPTY(name) (name##_read == name##_write)

#define BUFFER_IS_FULL(name) ((name##_write + 1) % BUFFER_SIZE == name##_read)

BUFFER_CREATE(receiver);
#define RECEIVER_READ(result) BUFFER_READ(receiver, result)
#define RECEIVER_WRITE(data) BUFFER_WRITE(receiver, data)
#define RECEIVER_IS_EMPTY BUFFER_IS_EMPTY(receiver)
#define RECEIVER_IS_FULL BUFFER_IS_FULL(receiver)

BUFFER_CREATE(transmitter);
#define TRANSMITTER_READ(result) BUFFER_READ(transmitter, result)
#define TRANSMITTER_WRITE(data) BUFFER_WRITE(transmitter, data)
#define TRANSMITTER_IS_EMPTY BUFFER_IS_EMPTY(transmitter)
#define TRANSMITTER_IS_FULL BUFFER_IS_FULL(transmitter)

#define ENABLE_DATA_REGISTER_EMPTY_INTERRUPT UCSR0B |= _BV(UDRIE0)
#define DISABLE_DATA_REGISTER_EMPTY_INTERRUPT UCSR0B &= ~_BV(UDRIE0)

// Receive Complete Interrupt
ISR(USART_RX_vect) {
    if (!RECEIVER_IS_FULL) {
        RECEIVER_WRITE(UDR0);
    }
}

// Transmit Complete Interrupt
ISR(USART_TX_vect) {
    // if (TRANSMITTER_IS_EMPTY) {
         // DISABLE_DATA_REGISTER_EMPTY_INTERRUPT;
    // }
}

// Data Register Empty Interrupt
ISR(USART_UDRE_vect) {
    if (!TRANSMITTER_IS_EMPTY) {
        TRANSMITTER_READ(UDR0);
    } else {
        // Jeśli nic więcej nie ma w buforze to wyłączamy przerwanie, żeby nie
        // obciążało programu (to przerwanie zaczyna wykonywać się przez cały
        // czas kiedy rejestr jest pusty).
        DISABLE_DATA_REGISTER_EMPTY_INTERRUPT;
    }
}

static int uart_transmit(char data, FILE* stream) {
    // Na skutek przerwania zmienić może się read pointer, ale nie wpłynie
    // to negatywnie na sprawdzenie, bo sam pointer jest atomowy, więc nie
    // utraci spójności, a wartość write się nie zmieni, więc najwyżej porównamy
    // wartość write z nowym read pointerem i skończymy pętlę wcześniej.
    while (TRANSMITTER_IS_FULL) { }

    cli();
    TRANSMITTER_WRITE(data);
    ENABLE_DATA_REGISTER_EMPTY_INTERRUPT;
    sei();

    return 0;
}

static int uart_receive(FILE* stream) {
    while (RECEIVER_IS_EMPTY) { }

    cli();
    BUFFER_DATA result;
    RECEIVER_READ(result);
    sei();

    return result;
}

FILE uart_file;

int main() {
    initialize_uart();

    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    sei();

    while (1) {
        printf("Hello world!\r\n");
        uint16_t input;
        scanf("%u", &input);
        printf("Foobar %u\r\n", input);
        _delay_ms(1000);
        for (uint8_t index = 0; index < 10; index++) {
            printf("foo\r\n");
            _delay_ms(100);
        }
        while (1) {
            printf("bar\r\n");
        }
    }
}
