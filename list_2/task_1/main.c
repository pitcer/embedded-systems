#include <avr/io.h>
#include <inttypes.h>
#include <util/delay.h>

#define BUTTON_PORT PORTC
#define BUTTON_PIN PINC
#define BUTTON PC4

#define LED_PORT PORTC
#define LED_DDR DDRC
#define LED PC5

#define BUFFER_SIZE 100

int main() {
    UCSR0B &= ~_BV(RXEN0) & ~_BV(TXEN0);

    LED_DDR |= _BV(LED);
    LED_PORT &= ~_BV(LED);

    BUTTON_PORT |= _BV(BUTTON);

    // Można zoptymalizować przechowując informacje na pojedyńczych bitach.
    uint8_t buffer[BUFFER_SIZE] = { 0 };
    uint8_t write_position = 0;
    while (1) {
        // Przesuń wartość pinu przycisku na najmłodszy bit.
        // Negujemy logicznie tą wartość, ponieważ jeśli przycisk był wciśnięty, to pin miał
        // wartość 0, a jeśli nie, to 1, zatem wartości te są odwrtone do tych, które chcemy
        // ustawić dla diody.
        buffer[write_position] = !((BUTTON_PIN & _BV(BUTTON)) >> BUTTON);
        // Zwiększamy pozycje bufora o 1.
        // Pozycja czytelnika jest zawsze o 1 większa od pisarza.
        write_position = (write_position + 1) % BUFFER_SIZE;
        // Wyzeruj bit, a potem ustaw go na wartość w buforze.
        LED_PORT = (LED_PORT & ~_BV(LED)) | buffer[write_position] << LED;
        _delay_ms(10);
    }
}
