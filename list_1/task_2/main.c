#include <avr/io.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>

#define LED PB5
#define LED_DDR DDRB
#define LED_PORT PORTB

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

#define UNIT 150

void enable_led() {
    LED_PORT |= _BV(LED);
}

void disable_led() {
    LED_PORT &= ~_BV(LED);
}

void delay_unit() {
    _delay_ms(UNIT);
}

void delay_letter() {
    _delay_ms(3 * UNIT);
}

void delay_word() {
    _delay_ms(7 * UNIT);
}

void display_dot() {
    enable_led();
    _delay_ms(UNIT);
    disable_led();
}

void display_dash() {
    enable_led();
    _delay_ms(3 * UNIT);
    disable_led();
}

#define DOT display_dot();
#define DASH display_dash();
#define SPACE delay_unit();

void handle_character(char character) {
    switch (character) {
    case 'a': {
        DOT SPACE DASH
    } break;
    case 'b': {
        DASH SPACE DOT SPACE DOT SPACE DOT
    } break;
    case 'c': {
        DASH SPACE DOT SPACE DASH SPACE DOT
    } break;
    case 'd': {
        DASH SPACE DOT SPACE DOT
    } break;
    case 'e': {
        DOT
    } break;
    case 'f': {
        DOT SPACE DOT SPACE DASH SPACE DOT
    } break;
    case 'g': {
        DASH SPACE DASH SPACE DOT
    } break;
    case 'h': {
        DOT SPACE DOT SPACE DOT SPACE DOT
    } break;
    case 'i': {
        DOT SPACE DOT
    } break;
    case 'j': {
        DOT SPACE DASH SPACE DASH SPACE DASH
    } break;
    case 'k': {
        DASH SPACE DOT SPACE DASH
    } break;
    case 'l': {
        DOT SPACE DASH SPACE DOT SPACE DOT
    } break;
    case 'm': {
        DASH SPACE DASH
    } break;
    case 'n': {
        DASH SPACE DOT
    } break;
    case 'o': {
        DASH SPACE DASH SPACE DASH
    } break;
    case 'p': {
        DOT SPACE DASH SPACE DASH SPACE DOT
    } break;
    case 'q': {
        DASH SPACE DASH SPACE DOT SPACE DASH
    } break;
    case 'r': {
        DOT SPACE DASH SPACE DOT
    } break;
    case 's': {
        DOT SPACE DOT SPACE DOT
    } break;
    case 't': {
        DASH
    } break;
    case 'u': {
        DOT SPACE DOT SPACE DASH
    } break;
    case 'v': {
        DOT SPACE DOT SPACE DOT SPACE DASH
    } break;
    case 'w': {
        DOT SPACE DASH SPACE DASH
    } break;
    case 'x': {
        DASH SPACE DOT SPACE DOT SPACE DASH
    } break;
    case 'y': {
        DASH SPACE DOT SPACE DASH SPACE DASH
    } break;
    case 'z': {
        DASH SPACE DASH SPACE DOT SPACE DOT
    } break;
    case ' ': {
        // Koniec poprzedniego znaku to 3 jednostki, tutaj czekamy 1 jednostkę, dodatkowo
        // na koniec czekamy 3 jednostki, razem 7 jednostek
        delay_unit();
    } break;
    default: {
    } break;
    }
    delay_letter();
}

int main() {
    // zainicjalizuj UART
    uart_init();
    // skonfiguruj strumienie wejścia/wyjścia
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    LED_DDR |= _BV(LED);
    while (1) {
        printf("Podaj słowo...\r\n");
        char string[256];
        fgets(string, 256, stdin);
        uint8_t length = strlen(string);

        for (uint8_t index = 0; index < length; index++) {
            char character = string[index];
            handle_character(character);
        }
    }
}
