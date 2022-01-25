#include <avr/io.h>
#include <math.h>
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

#define THERMISTOR_MUX ADC0D

// inicjalizacja ADC
static inline void initialize_adc(void) {
    ADMUX = _BV(REFS1) | _BV(REFS0); // referencja AVcc = 1.1V
    ADMUX |= THERMISTOR_MUX; // pomiar ADC0
    DIDR0 = _BV(THERMISTOR_MUX); // wyłącz wejście cyfrowe na ADC0
    // częstotliwość zegara ADC 125 kHz (16 MHz / 128)
    ADCSRA = _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); // preskaler 128
    ADCSRA |= _BV(ADEN); // włącz ADC
}

// T = ~20.8 C = 293.95 K

// ADC = V_in * 1024 / V_ref
// V_t = ADC * V_ref / 1024
// ADC = 568
// V_t = 568 * 5V / 1024 = 2.7734375 V

// V_t = V_in * R_t / (R_t + R)
// R_t = V_t * R / (V_in - V_t)
// R_t = 5480.701754385966 O

// B = ln(R/R_0) / (1/T - 1/T_0)
// B = 3206.637682344697

static inline uint16_t read_adc(void) {
    ADCSRA |= _BV(ADSC); // wykonaj konwersję
    loop_until_bit_is_set(ADCSRA, ADIF); // czekaj na wynik
    ADCSRA |= _BV(ADIF); // wyczyść bit ADIF (pisząc 1!)
    return ADC; // weź zmierzoną wartość (0..1023)
}

int main() {
    // zainicjalizuj UART
    uart_init();
    // skonfiguruj strumienie wejścia/wyjścia
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    initialize_adc();

    while (1) {
        uint16_t adc = read_adc();
        printf("%" PRIu16 "\r\n", adc);
        const float vref = 1.1;
        float thermistor_voltage = (adc * vref) / 1024;
        printf("%f\r\n", thermistor_voltage);
        // R = U/I
        // 1/(10^(-6) * 130)
        float thermistor_resistance = thermistor_voltage * 7692.307;
        printf("%f\r\n", thermistor_resistance);
        const float T_0 = 298.15;
        const float R_0 = 4700;
        // const float thermistor_constant = 3206.637682344697;
        const float thermistor_constant = 5650.711391225834;
        float temperature = thermistor_constant / (log(thermistor_resistance / R_0) + (thermistor_constant / T_0));
        float temperature_celsius = temperature - 273.15;
        printf("Odczytano: %f°C\r\n", temperature_celsius);
        _delay_ms(1000);
    }
}

//736 === 21.0 stC
