#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>

static void initialize_timer_1() {
    // ustaw tryb licznika
    // COM1A = 10   -- non-inverting mode
    // WGM1  = 1110 -- fast PWM top=ICR1
    // CS1   = 101  -- prescaler 256
    // ICR1  = 624
    // częstotliwość 16e6/(256*(1+624)) = 100 Hz
    ICR1 = 624;
    TCCR1A = _BV(COM1A1) | _BV(WGM11);
    TCCR1B = _BV(WGM12) | _BV(WGM13) | _BV(CS12);
    // odmaskowanie przerwania przepełnienia licznika
    TIMSK1 |= _BV(TOIE1);
}

#define BUTTON_PORT PORTD
#define BUTTON_PIN PIND
#define BUTTON PD2

#define LED_PORT PORTB
#define LED_DDR DDRB
#define LED PB4

#define BUFFER_SIZE 100

static volatile uint8_t BUFFER[BUFFER_SIZE] = { 0 };

static volatile uint8_t write_position = 0;

// procedura obsługi przerwania przepełnienia licznika
ISR(TIMER1_OVF_vect) {
    // Przesuń wartość pinu przycisku na najmłodszy bit.
    // Negujemy logicznie tą wartość, ponieważ jeśli przycisk był wciśnięty, to pin miał
    // wartość 0, a jeśli nie, to 1, zatem wartości te są odwrtone do tych, które chcemy
    // ustawić dla diody.
    BUFFER[write_position] = !((BUTTON_PIN & _BV(BUTTON)) >> BUTTON);
    // Zwiększamy pozycje bufora o 1.
    // Pozycja czytelnika jest zawsze o 1 większa od pisarza.
    write_position = (write_position + 1) % BUFFER_SIZE;
    // Wyzeruj bit, a potem ustaw go na wartość w buforze.
    LED_PORT = (LED_PORT & ~_BV(LED)) | BUFFER[write_position] << LED;
}

int main() {
    LED_DDR |= _BV(LED);
    LED_PORT &= ~_BV(LED);

    BUTTON_PORT |= _BV(BUTTON);

    initialize_timer_1();

    // ustaw tryb uśpienia na tryb bezczynności
    set_sleep_mode(SLEEP_MODE_IDLE);

    // odmaskuj przerwania
    sei();

    while (1) {
        sleep_mode();
    }
}
