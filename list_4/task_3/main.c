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

FILE uart_file;

/* ```julia
using Pipe

@pipe range(0, log(256), length=256) |>
      map(x -> trunc(Int, round(exp(x))) - 1, _) |>
      foreach(x -> print("$x, "), _)
``` */
/*static const uint8_t exponential_table[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 10, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14, 14, 14, 15, 15, 16, 16, 16, 17, 17, 17, 18, 18, 19, 19, 20, 20, 20, 21, 21, 22, 22, 23, 23, 24, 25, 25, 26, 26, 27, 27, 28, 29, 29, 30, 31, 31, 32, 33, 34, 34, 35, 36, 37, 38, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 54, 55, 56, 57, 59, 60, 61, 63, 64, 65, 67, 68, 70, 72, 73, 75, 76, 78, 80, 82, 83, 85, 87, 89, 91, 93, 95, 97, 99, 102, 104, 106, 109, 111, 114, 116, 119, 121, 124, 127, 129, 132, 135, 138, 141, 144, 148, 151, 154, 158, 161, 165, 168, 172, 176, 180, 184, 188, 192, 196, 201, 205, 209, 214, 219, 224, 229, 234, 239, 244, 249, 255
    // 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 15, 15, 16, 17, 18, 18, 19, 20, 21, 22, 23, 24, 25, 27, 28, 29, 30, 32, 33, 35, 36, 38, 40, 42, 44, 46, 48, 50, 52, 55, 57, 60, 62, 65, 68, 71, 74, 78, 81, 85, 89, 93, 97, 101, 106, 111, 116, 121, 126, 132, 138, 144, 151, 157, 164, 172, 180, 188, 196, 205, 214, 224, 234, 244, 255
    // 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 253, 253, 253, 253, 253, 253, 253, 253, 252, 252, 252, 252, 252, 252, 251, 251, 251, 251, 251, 250, 250, 250, 249, 249, 249, 249, 248, 248, 248, 247, 247, 246, 246, 245, 245, 244, 244, 243, 243, 242, 242, 241, 240, 240, 239, 238, 237, 237, 236, 235, 234, 233, 232, 231, 230, 228, 227, 226, 225, 223, 222, 220, 219, 217, 215, 213, 211, 209, 207, 205, 203, 200, 198, 195, 193, 190, 187, 184, 181, 177, 174, 170, 166, 162, 158, 154, 149, 144, 139, 134, 129, 123, 117, 111, 104, 98, 91, 83, 75, 67, 59, 50, 41, 31, 21, 11, 0
};*/

/* ```julia
using Pipe

length = 256
exponentials = @pipe
    range(0, log(256), length=length) |>
    map(x -> exp(x) - 1, _)
sines = @pipe
    range(-π/2, π/2, length=length) |>
    map(x -> (sin(x) + 1) / 2, _)
@pipe zip(exponentials, sines) |>
      map(pair -> first(pair) * last(pair), _) |>
      map(x -> trunc(Int, round(x)), _) |>
      foreach(x -> print("$x, "), _)
``` */
static const uint8_t covers_table[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 10, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14, 14, 14, 15, 15, 16, 16, 16, 17, 17, 17, 18, 18, 19, 19, 20, 20, 20, 21, 21, 22, 22, 23, 23, 24, 25, 25, 26, 26, 27, 27, 28, 29, 29, 30, 31, 31, 32, 33, 34, 34, 35, 36, 37, 38, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 54, 55, 56, 57, 59, 60, 61, 63, 64, 65, 67, 68, 70, 72, 73, 75, 76, 78, 80, 82, 83, 85, 87, 89, 91, 93, 95, 97, 99, 102, 104, 106, 109, 111, 114, 116, 119, 121, 124, 127, 129, 132, 135, 138, 141, 144, 148, 151, 154, 158, 161, 165, 168, 172, 176, 180, 184, 188, 192, 196, 201, 205, 209, 214, 219, 224, 229, 234, 239, 244, 249, 255
    // 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 12, 12, 12, 13, 13, 14, 14, 15, 15, 15, 16, 17, 17, 18, 18, 19, 19, 20, 21, 21, 22, 23, 23, 24, 25, 25, 26, 27, 28, 29, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 42, 43, 44, 45, 47, 48, 49, 51, 52, 53, 55, 56, 58, 60, 61, 63, 64, 66, 68, 70, 72, 74, 75, 77, 79, 82, 84, 86, 88, 90, 93, 95, 97, 100, 102, 105, 108, 110, 113, 116, 119, 122, 125, 128, 131, 134, 137, 141, 144, 148, 151, 155, 158, 162, 166, 170, 174, 178, 182, 186, 191, 195, 200, 204, 209, 214, 218, 223, 228, 234, 239, 244, 249, 255
};

#define TIMER_RED_TOP 255
#define TIMER_GREEN_TOP 255
#define TIMER_BLUE_TOP 255

#define TIMER_RED_OCR OCR0B
#define TIMER_GREEN_OCR OCR1B
#define TIMER_BLUE_OCR OCR1A

static void initialize_timer_0() {
    // COM0B = 10  -- non-inverting mode
    // WGM0  = 011 -- fast PWM, top=0xFF
    // CS0   = 001 -- prescaler 1
    // OCR0  = 255
    // OCR0A = TIMER_RED_TOP;
    TCCR0A = _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
    TCCR0B = _BV(CS00);
    DDRD |= _BV(PD5);
}

static void initialize_timer_1() {
    // ustaw tryb licznika
    // COM1A = 10   -- non-inverting mode
    // COM1B = 10   -- non-inverting mode
    // WGM1  = 0101 -- Fast PWM, 8-bit, top=0xFF
    // CS1   = 001  -- prescaler 1

    TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM10);
    TCCR1B = _BV(WGM12) | _BV(CS10);
    DDRB |= _BV(PB1);
    DDRB |= _BV(PB2);
}

#define DELAY 5

#define RUN_CYCLIC(index, index_lower, index_upper, action) \
    for (; index < index_upper; index++) {                  \
        action;                                             \
        _delay_ms(DELAY);                                   \
    }                                                       \
    for (; index > index_lower; index--) {                  \
        action;                                             \
        _delay_ms(DELAY);                                   \
    }

// https://en.wikipedia.org/wiki/Linear_congruential_generator
static uint8_t random_state;

static uint8_t next_random() {
    return random_state = 181 * random_state + 1;
}

static void initialize_random(uint8_t seed) {
    random_state = 2 * seed + 1;
}

static void generate_random_color(uint8_t rgb[3]) {
    uint8_t random = next_random();
    uint8_t index = random % 3;
    rgb[index] = 0;
    uint8_t r = next_random() % 2;
    if (index == 0) {
        rgb[1] = r ? 255 : random;
        rgb[2] = r ? random : 255;
    } else if (index == 1) {
        rgb[0] = r ? 255 : random;
        rgb[2] = r ? random : 255;
    } else if (index == 2) {
        rgb[0] = r ? 255 : random;
        rgb[1] = r ? random : 255;
    }
    //rgb[(index + 1) % 3] = 255;
    //rgb[(index + 2) % 3] = random;
}

static uint16_t get_noise_from_adc() {
    ADMUX = _BV(REFS0); // referencja AVcc
    DIDR0 = _BV(ADC0D); // wyłącz wejście cyfrowe na ADC0
    ADCSRA |= _BV(ADEN); // włącz ADC
    ADCSRA |= _BV(ADSC); // wykonaj konwersję
    loop_until_bit_is_set(ADCSRA, ADIF); // czekaj na wynik
    ADCSRA |= _BV(ADIF); // wyczyść bit ADIF (pisząc 1!)
    ADCSRA &= ~_BV(ADEN); // wyłącz ADC
    return ADC; // weź zmierzoną wartość (0..1023)
}

int main() {
    // zainicjalizuj UART
    // uart_init();
    // // skonfiguruj strumienie wejścia/wyjścia
    // fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    // stdin = stdout = stderr = &uart_file;

    uint8_t seed = get_noise_from_adc();
    initialize_random(seed);

    initialize_timer_0();
    initialize_timer_1();

    uint8_t index = 0;
    uint8_t rgb[3];
    while (1) {
        generate_random_color(rgb);
        RUN_CYCLIC(
            index, 0, sizeof(covers_table) - 1,
            uint8_t cover = covers_table[index];
            TIMER_RED_OCR = 255 - (rgb[0] * cover / 255);
            TIMER_GREEN_OCR = 255 - (rgb[1] * cover / 255);
            TIMER_BLUE_OCR = 255 - (rgb[2] * cover / 255););
    }
}
