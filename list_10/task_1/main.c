#include <avr/io.h>
#include <util/delay.h>

#define POTENTIOMETER_MUX ADC0D

static inline void initialize_adc(void) {
    ADMUX = _BV(REFS0); // referencja AVcc = 5V
    // pomiar ADC3
    ADMUX |= POTENTIOMETER_MUX;
    // wyłącz wejścia cyfrowe na ADC3
    DIDR0 = _BV(POTENTIOMETER_MUX);
    // częstotliwość zegara ADC 125 kHz (16 MHz / 128)
    ADCSRA = _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); // preskaler 128
    ADCSRA |= _BV(ADEN); // włącz ADC
}

static inline uint16_t read_adc(void) {
    ADCSRA |= _BV(ADSC); // wykonaj konwersję
    loop_until_bit_is_set(ADCSRA, ADIF); // czekaj na wynik
    ADCSRA |= _BV(ADIF); // wyczyść bit ADIF (pisząc 1!)
    return ADC; // weź zmierzoną wartość (0..1023)
}

#define ADC_MAXIMUM 1023

static inline void initialize_timer(void) {
    // ustaw tryb licznika
    // COM1A = 10   -- non-inverting mode
    // WGM1  = 1110 -- fast PWM top=ICR1
    // CS1   = 010  -- prescaler 8
    // ICR1  = 2046
    // częstotliwość 16e6/(8*(1+2046)) = 977 Hz
    // wzór: datasheet 20.12.3 str. 164

    ICR1 = ADC_MAXIMUM * 2;
    TCCR1A = _BV(COM1A1) | _BV(WGM11);
    TCCR1B = _BV(WGM12) | _BV(WGM13) | _BV(CS11);
    // ustaw pin OC1A (PB1) jako wyjście
    DDRB |= _BV(PB1);
}

int main(void) {
    initialize_adc();
    initialize_timer();

    while (1) {
        uint16_t adc = read_adc();
        OCR1A = adc * 2;
        _delay_ms(100);
    }
}
