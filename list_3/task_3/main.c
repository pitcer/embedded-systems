#include <avr/delay.h>
#include <avr/io.h>
#include <inttypes.h>

#define LED PB5
#define LED_DDR DDRB
#define LED_PORT PORTB

// inicjalizacja ADC
void adc_init() {
    ADMUX = _BV(REFS0); // referencja AVcc = 5V, pomiar ADC0

    DIDR0 = _BV(ADC0D); // wyłącz wejście cyfrowe na ADC0
    // częstotliwość zegara ADC 125 kHz (16 MHz / 128)
    ADCSRA = _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); // preskaler 128
    ADCSRA |= _BV(ADEN); // włącz ADC
}

inline static uint16_t read_adc() {
    ADCSRA |= _BV(ADSC); // wykonaj konwersję
    loop_until_bit_is_set(ADCSRA, ADIF); // czekaj na wynik
    ADCSRA |= _BV(ADIF); // wyczyść bit ADIF (pisząc 1!)
    return ADC; // weź zmierzoną wartość (0..1023)
}

// ```julia
// using Pipe
// @pipe range(0, 9, length=128) |>
// map(x -> trunc(Int, floor(exp(x))) - 1, _) |>
// foreach(x -> print("$x, "), _)
// ```
static const uint16_t exponential_table[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 7,
    8, 9, 10, 10, 11, 12, 13, 14, 16, 17, 18, 20, 21, 23, 25, 26, 29, 31, 33, 36, 38, 41, 44, 48,
    51, 55, 59, 64, 69, 74, 79, 85, 92, 99, 106, 114, 122, 131, 141, 152, 163, 175, 188, 202, 217,
    233, 250, 269, 288, 310, 332, 357, 383, 412, 442, 474, 509, 547, 587, 630, 677, 727, 780, 838,
    899, 965, 1036, 1113, 1194, 1282, 1376, 1478, 1586, 1703, 1828, 1962, 2107, 2261, 2428, 2606,
    2797, 3003, 3224, 3461, 3715, 3988, 4281, 4595, 4933, 5295, 5684, 6101, 6550, 7031, 7547, 8102
};

int main() {
    adc_init();
    LED_DDR |= _BV(LED);

    while (1) {
        uint16_t adc = read_adc();
        const uint16_t adc_maximum_value = 1024;
        const uint16_t exponential_length = sizeof(exponential_table) / 2;
        uint16_t iterations = exponential_table[adc / (adc_maximum_value / exponential_length)];
        LED_PORT |= _BV(LED);
        for (uint16_t index = 0; index < iterations; index++) {
            _delay_us(1);
        }
        LED_PORT &= ~_BV(LED);
        const uint16_t maximum_iterations = exponential_table[sizeof(exponential_table) / 2 - 1];
        for (uint16_t index = 0; index < maximum_iterations - iterations; index++) {
            _delay_us(1);
        }
    }
}
