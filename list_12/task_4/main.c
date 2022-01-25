#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>

#define BAUD 9600 // baudrate
#define UBRR_VALUE ((F_CPU) / 16 / (BAUD)-1) // zgodnie ze wzorem

// inicjalizacja UART
static inline void uart_init(void) {
    // ustaw baudrate
    UBRR0 = UBRR_VALUE;
    // włącz odbiornik i nadajnik
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);
    // ustaw format 8n1
    UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);
}

// transmisja jednego znaku
static int uart_transmit(char data, FILE* stream) {
    // czekaj aż transmiter gotowy
    while (!(UCSR0A & _BV(UDRE0)))
        ;
    UDR0 = data;
    return 0;
}

// odczyt jednego znaku
static int uart_receive(FILE* stream) {
    // czekaj aż znak dostępny
    while (!(UCSR0A & _BV(RXC0)))
        ;
    return UDR0;
}

FILE uart_file;

#define CAPACITOR_MUX ADC2D

static inline void initialize_adc(void) {
    ADMUX = _BV(REFS0); // referencja AVcc = 5V
    ADMUX |= CAPACITOR_MUX; // pomiar ADC2
    DIDR0 = _BV(CAPACITOR_MUX); // wyłącz wejście cyfrowe na ADC2
    // częstotliwość zegara ADC 125 kHz (16 MHz / 128)
    // ADCSRA = _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); // preskaler 128
    // częstotliwość zegara ADC 500 kHz (16 MHz / 32)
    ADCSRA = _BV(ADPS2) | _BV(ADPS0); // preskaler 32
    ADCSRA |= _BV(ADIE); // włącz przerwanie
    // TODO: przetestuj auto-trigger
    ADCSRA |= _BV(ADATE); // auto-trigger
    ADCSRB |= _BV(ADTS2) | _BV(ADTS1); // auto-trigger on timer 1 overflow
    ADCSRA |= _BV(ADEN); // włącz ADC
}

#define set_bit(reg, index) (reg) |= _BV(index);
#define clear_bit(reg, index) (reg) &= ~_BV(index);
#define get_bit(reg, index) ((reg)&_BV(index))

#define START_ADC_CONVERSION set_bit(ADCSRA, ADSC);

#define SAMPLES 100

static volatile uint16_t measurements[SAMPLES];
static volatile uint16_t measurements_count = 0;

ISR(ADC_vect) {
    if (measurements_count < SAMPLES) {
        measurements[measurements_count] = ADC;
        measurements_count++;
    }
}

static inline uint16_t read_adc(void) {
    ADCSRA |= _BV(ADSC); // wykonaj konwersję
    loop_until_bit_is_set(ADCSRA, ADIF); // czekaj na wynik
    ADCSRA |= _BV(ADIF); // wyczyść bit ADIF (pisząc 1!)
    return ADC; // weź zmierzoną wartość (0..1023)
}

static inline uint32_t adc_to_milivolts(uint32_t adc) {
    const uint32_t vref = 5;
    return (adc * vref * 1000) / 1024;
}

static inline void initialize_timer(void) {
    // ustaw tryb licznika
    // COM1A = 10   -- non-inverting mode
    // WGM1  = 1110 -- Fast PWM top=ICR1
    // CS1   = 010  -- prescaler 8
    // ICR1  = 16*10^6/(frequency*8)-1
    // częstotliwość 16e6/(8*(1+ICR1) = frequency Hz

    // przykładowa konfiguracja
    // ICR1  = 199
    // częstotliwość 16e6/(8*(1+199)) = 10 kHz
    // okres 10 us

    TCCR1A = _BV(COM1A1);
    TCCR1B = _BV(CS11);

    TCCR1A |= _BV(WGM11);
    TCCR1B |= _BV(WGM13) | _BV(WGM12);

    TIMSK1 |= _BV(TOIE1);
}

ISR(TIMER1_OVF_vect) {
    // TODO: przetestuj auto-trigger
    // START_ADC_CONVERSION;
}

#define DUMPER_PORT PORTD
#define DUMPER_DDR DDRD
#define DUMPER_PIN PIND
#define DUMPER PD4

int main(void) {
    uart_init();
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    initialize_adc();

    printf("Podaj częstotliwość próbkowania...\r\n");
    uint16_t frequency;
    scanf("%" SCNu16, &frequency);
    ICR1 = (uint32_t)16000000 / ((uint32_t)frequency * 8) - 1;
    initialize_timer();

    _delay_ms(1000);

    uint16_t before_uncharge = read_adc();
    uint32_t before_uncharge_milivolts = adc_to_milivolts(before_uncharge);
    printf("Before uncharge: %" PRIu32 "mV (%" PRIu16 ")\r\n", before_uncharge_milivolts, before_uncharge);

    // włącz wyjście i ustaw na 0V aby rozładować kondensator
    DUMPER_PORT &= ~_BV(DUMPER);
    DUMPER_DDR |= _BV(DUMPER);

    _delay_ms(1000);

    uint16_t after_uncharge = read_adc();
    uint32_t after_uncharge_milivolts = adc_to_milivolts(after_uncharge);
    printf("After uncharge: %" PRIu32 "mV (%" PRIu16 ")\r\n", after_uncharge_milivolts, after_uncharge);

    // wyłącz wyjście aby przestać rozładowywać kondensator
    DUMPER_DDR &= ~_BV(DUMPER);
    TCNT1 = 0;
    sei();
    START_ADC_CONVERSION;

    _delay_ms(3000);
    cli();
    for (uint8_t index = 0; index < measurements_count; index++) {
        uint16_t measurement = measurements[index];
        uint32_t milivolts = adc_to_milivolts(measurement);
        printf("%" PRIu32 "mV (%" PRIu16 "), ", milivolts, measurement);
    }
    printf("\r\nMeasurements: %u\r\n", measurements_count);

    uint16_t after_charge = read_adc();
    uint32_t after_charge_milivolts = adc_to_milivolts(after_charge);
    printf("After charge: %" PRIu32 "mV (%" PRIu16 ")\r\n", after_charge_milivolts, after_charge);

    uint8_t last_sample_index = 0;
    for (uint8_t index = 0; index < measurements_count; index++) {
        uint16_t measurement = measurements[index];
        // jeśli próbka jest mniejsza niż napięcie początkowe - ~100mV
        // to wtedy użyj jej do obliczenia pojemności
        if (measurement < before_uncharge - 20) {
            measurements[index] = adc_to_milivolts(measurement);
            last_sample_index = index;
        } else {
            break;
        }
    }

    uint8_t deltas_size = last_sample_index;
    uint16_t deltas[deltas_size];
    for (uint8_t index = 0; index < deltas_size; index++) {
        uint16_t sample = measurements[index];
        uint16_t next_sample = measurements[index + 1];
        deltas[index] = next_sample - sample;
    }

    uint32_t avegare_delta_milivolts = 0;
    for (uint8_t index = 0; index < deltas_size; index++) {
        avegare_delta_milivolts += deltas[index];
    }
    avegare_delta_milivolts /= deltas_size;

    //     130[uA] * dt[uS]   130 * 10^(-6) * dt * 10^(-6)
    // C = ---------------- = ---------------------------- =
    //         dU[mV]                 dU * 10^(-3)
    //
    //     130 * dt * 10^(-9)   130 * dt * 10^3
    //   = ------------------ = --------------- [pF]
    //            dU                  dU

    uint32_t period_microseconds = 1000000 / frequency;
    uint32_t capacity_picofarads = (130 * period_microseconds * 1000) / avegare_delta_milivolts;

    printf("Capacity: %" PRIu32 " pF\r\n", capacity_picofarads);

    while (1) { }
}

// 10 nF  - 20 kHz
// 100 nF - 20 kHz
// 100 uF - 32 Hz
