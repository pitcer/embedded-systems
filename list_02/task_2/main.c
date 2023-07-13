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

FILE uart_file;

#define BUTTON_PORT PORTC
#define BUTTON_PIN PINC
#define BUTTON PC4

#define LED_PORT PORTC
#define LED_DDR DDRC
#define LED PC5

#define TICK_TIME 10 // 10 ms

#define MASK_GET(mask, bit) ((mask)&_BV(bit))
#define MASK_SET(mask, bit) (mask) |= _BV(bit)
#define MASK_UNSET(mask, bit) (mask) &= ~_BV(bit)

#define HANDLE_BUTTON_PRESSED(handler)          \
    /* przycisk 1 -> pin 0, maska 0 */          \
    if (!MASK_GET(BUTTON_PIN | mask, BUTTON)) { \
        _delay_ms(TICK_TIME);                   \
        INCREASE_COUNTER(1);                    \
        /* czy pin nadal jest zgaszony */       \
        if (!MASK_GET(BUTTON_PIN, BUTTON)) {    \
            MASK_SET(mask, BUTTON_PRESSED);     \
            handler                             \
        }                                       \
    }

#define HANDLE_BUTTON_HELD(handler)                     \
    /* przycisk 1 -> pin 0, maska 1 */                  \
    if (MASK_GET((BUTTON_PIN ^ mask) & mask, BUTTON)) { \
        _delay_ms(TICK_TIME);                           \
        INCREASE_COUNTER(1);                            \
        /* czy pin nadal jest zgaszony */               \
        if (!MASK_GET(BUTTON_PIN, BUTTON)) {            \
            handler                                     \
        }                                               \
    }

#define HANDLE_BUTTON_RELEASED(handler)        \
    /* przycisk 0 -> pin 1, maska 1 */         \
    if (MASK_GET(BUTTON_PIN & mask, BUTTON)) { \
        _delay_ms(TICK_TIME);                  \
        INCREASE_COUNTER(1);                   \
        /* czy pin nadal jest zapalony */      \
        if (MASK_GET(BUTTON_PIN, BUTTON)) {    \
            MASK_UNSET(mask, BUTTON_PRESSED);  \
            handler                            \
        }                                      \
    }

#define HANDLE_BUTTON_STILL(handler)                          \
    /* przycisk 0 -> pin 1, maska 0 */                        \
    if (MASK_GET((BUTTON_PIN ^ mask) & BUTTON_PIN, BUTTON)) { \
        _delay_ms(TICK_TIME);                                 \
        INCREASE_COUNTER(1);                                  \
        /* czy pin nadal jest zapalony */                     \
        if (MASK_GET(BUTTON_PIN, BUTTON)) {                   \
            handler                                           \
        }                                                     \
    }

#define IN_RANGE(start, number, end_exclusive) number >= start&& number < end_exclusive

#define INCREASE_COUNTER(amount)           \
    if (MASK_GET(mask, COUNTER_ENABLED)) { \
        counter += amount;                 \
    }

// Kodowanie liter: xxxxxsss_2
// gdzie s oznacza bity odpowiadające za długość liczby
//       x oznacza bity kodujące znaki idąc od najmłodszych bitów
//         * 0 <-> .
//         * 1 <-> -

#define LETTER_ADD_DOT \
    letter++

#define LETTER_ADD_DASH                 \
    uint8_t size = letter & 0b00000111; \
    letter |= _BV(3 + size);            \
    letter++;

#define ENABLE_LED MASK_SET(LED_PORT, LED)
#define DISABLE_LED MASK_UNSET(LED_PORT, LED)

static char morse_letter_to_character(uint8_t letter) {
    switch (letter) {
    case 0b00010010: // .-
        return 'a';
    case 0b00001100: // -...
        return 'b';
    case 0b00101100: // -.-.
        return 'c';
    case 0b00001011: // -..
        return 'd';
    case 0b00000001: // .
        return 'e';
    case 0b00100100: // ..-.
        return 'f';
    case 0b00011011: // --.
        return 'g';
    case 0b00000100: // ....
        return 'h';
    case 0b00000010: // ..
        return 'i';
    case 0b01110100: // .---
        return 'j';
    case 0b00101011: // -.-
        return 'k';
    case 0b00010100: // .-..
        return 'l';
    case 0b00011010: // --
        return 'm';
    case 0b00001010: // -.
        return 'n';
    case 0b00111011: // ---
        return 'o';
    case 0b00110100: // .--.
        return 'p';
    case 0b01011100: // --.-
        return 'q';
    case 0b00010011: // .-.
        return 'r';
    case 0b00000011: // ...
        return 's';
    case 0b00001001: // -
        return 't';
    case 0b00100011: // ..-
        return 'u';
    case 0b01000100: // ...-
        return 'v';
    case 0b00110011: // .--
        return 'w';
    case 0b01001100: // -..-
        return 'x';
    case 0b01101100: // -.--
        return 'y';
    case 0b00011100: // --..
        return 'z';
    default:
        return '?';
    }
}

#define COUNTER_ENABLED 0
#define DOT 1
#define DASH 2
#define FLUSH_BUFFER 3
#define BUTTON_PRESSED BUTTON // 4
#define NEXT_CHARACTER 5
#define NEXT_LETTER 6
#define NEXT_WORD 7

#define UNIT 25 // 250 ms

int main() {
    // zainicjalizuj UART
    uart_init();
    // skonfiguruj strumienie wejścia/wyjścia
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    LED_DDR |= _BV(LED);
    LED_PORT &= ~_BV(LED);

    BUTTON_PORT |= _BV(BUTTON);

    char text_buffer[256];
    uint8_t text_buffer_length = 0;

    uint8_t mask = 0x0;
    uint8_t counter = 0;
    uint8_t letter = 0x0;
    while (1) {
        HANDLE_BUTTON_PRESSED(
            counter = 0;
            MASK_SET(mask, COUNTER_ENABLED);
            DISABLE_LED;
            if (MASK_GET(mask, NEXT_CHARACTER)) {
                MASK_UNSET(mask, NEXT_CHARACTER);
            } if (MASK_GET(mask, NEXT_LETTER)) {
                MASK_UNSET(mask, NEXT_LETTER);
            } if (MASK_GET(mask, NEXT_WORD)) {
                MASK_UNSET(mask, NEXT_WORD);
                text_buffer[text_buffer_length++] = ' ';
            } if (MASK_GET(mask, FLUSH_BUFFER)) {
                MASK_UNSET(mask, FLUSH_BUFFER);
            })

        HANDLE_BUTTON_HELD(
            // przerwa powinna trwać przynajmniej 1 jednostkę, ale wygodniej jest
            // kiedy można pisać od razu
            // if (IN_RANGE(UNIT, counter, 3 * UNIT) && !MASK_GET(mask, DOT)) {
            if (counter < 3 * UNIT && !MASK_GET(mask, DOT)) {
                MASK_SET(mask, DOT);
            } else if (3 * UNIT <= counter && !MASK_GET(mask, DASH)) {
                MASK_UNSET(mask, DOT);
                MASK_SET(mask, DASH);
                MASK_UNSET(mask, COUNTER_ENABLED);
                ENABLE_LED;
            })

        HANDLE_BUTTON_RELEASED(
            counter = 0;
            MASK_SET(mask, COUNTER_ENABLED);
            DISABLE_LED;
            if (MASK_GET(mask, DOT)) {
                LETTER_ADD_DOT;
                MASK_UNSET(mask, DOT);
                putchar('.');
            } if (MASK_GET(mask, DASH)) {
                LETTER_ADD_DASH;
                MASK_UNSET(mask, DASH);
                putchar('-');
            })

        HANDLE_BUTTON_STILL(
            // przerwa powinna trwać przynajmniej 1 jednostkę, ale wygodniej jest
            // kiedy można pisać od razu
            // if (IN_RANGE(UNIT, counter, 3 * UNIT) && !MASK_GET(mask, NEXT_CHARACTER)) {
            if (counter < 3 * UNIT && !MASK_GET(mask, NEXT_CHARACTER)) {
                MASK_SET(mask, NEXT_CHARACTER);
            } else if (IN_RANGE(3 * UNIT, counter, 7 * UNIT) && !MASK_GET(mask, NEXT_LETTER)) {
                MASK_UNSET(mask, NEXT_CHARACTER);
                MASK_SET(mask, NEXT_LETTER);
                ENABLE_LED;
                text_buffer[text_buffer_length++] = morse_letter_to_character(letter);
                letter = 0;
                putchar(' ');
            } else if (IN_RANGE(7 * UNIT, counter, 10 * UNIT) && !MASK_GET(mask, NEXT_WORD)) {
                MASK_SET(mask, NEXT_WORD);
                DISABLE_LED;
                putchar(' ');
                putchar(' ');
                putchar(' ');
            } else if (10 * UNIT <= counter && !MASK_GET(mask, FLUSH_BUFFER)) {
                MASK_UNSET(mask, NEXT_WORD);
                MASK_SET(mask, FLUSH_BUFFER);
                MASK_UNSET(mask, COUNTER_ENABLED);
                putchar('\r');
                putchar('\n');
                fwrite(text_buffer, text_buffer_length, 1, stdout);
                putchar('\r');
                putchar('\n');
                fflush(stdout);
                text_buffer_length = 0;
            })

        _delay_ms(TICK_TIME);
        INCREASE_COUNTER(1);
    }
}
