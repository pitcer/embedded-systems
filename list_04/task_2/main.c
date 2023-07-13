#include <avr/io.h>

/* ```julia
using Pipe

@pipe range(0, log(1024), length=256) |>
      map(x -> trunc(Int, round(exp(x))) - 1, _) |>
      foreach(x -> print("$x, "), _)
``` */
static const uint16_t exponential_table[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14, 15, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 21, 21, 22, 22, 23, 24, 24, 25, 26, 27, 27, 28, 29, 30, 31, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 42, 43, 44, 45, 46, 48, 49, 50, 52, 53, 55, 56, 58, 60, 61, 63, 65, 67, 68, 70, 72, 74, 76, 79, 81, 83, 85, 88, 90, 93, 95, 98, 101, 103, 106, 109, 112, 115, 119, 122, 125, 129, 132, 136, 140, 144, 148, 152, 156, 160, 165, 169, 174, 179, 184, 189, 194, 199, 205, 211, 216, 222, 229, 235, 241, 248, 255, 262, 269, 277, 284, 292, 300, 309, 317, 326, 335, 344, 354, 364, 374, 384, 394, 405, 417, 428, 440, 452, 465, 477, 491, 504, 518, 532, 547, 562, 578, 594, 610, 627, 644, 662, 680, 699, 718, 738, 758, 779, 801, 823, 846, 869, 893, 918, 943, 969, 996, 1023
};

static void initialize_adc() {
    ADMUX = _BV(REFS0); // referencja AVcc = 5V, pomiar ADC0
    DIDR0 = _BV(ADC0D); // wyłącz wejście cyfrowe na ADC0
    // częstotliwość zegara ADC 125 kHz (16 MHz / 128)
    ADCSRA = _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); // preskaler 128
    ADCSRA |= _BV(ADEN); // włącz ADC
}

#define TOP 1023
#define OCR OCR1A

static void initialize_timer() {
    // COM1A = 10   -- non-inverting mode
    // WGM1  = 1110 -- fast PWM top=ICR1
    // CS1   = 001  -- prescaler 1
    // ICR1  = 1023

    ICR1 = TOP;
    TCCR1A = _BV(COM1A1) | _BV(WGM11);
    TCCR1B = _BV(WGM12) | _BV(WGM13) | _BV(CS10);
    DDRB |= _BV(PB1);
}

// rezystor 10kO

int main() {
    initialize_adc();
    initialize_timer();

    while (1) {
        ADCSRA |= _BV(ADSC); // wykonaj konwersję
        loop_until_bit_is_set(ADCSRA, ADIF); // czekaj na wynik
        ADCSRA |= _BV(ADIF); // wyczyść bit ADIF (pisząc 1!)
        uint16_t adc = ADC; // weź zmierzoną wartość (0..1023)
        OCR = exponential_table[adc / 4];
    }
}
