#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#define DISABLE_UNCOMMON_PITCHES
#undef DISABLE_UNCOMMON_PITCHES

#define BUZZ PC0
#define BUZZ_DDR DDRC
#define BUZZ_PORT PORTC

#define LED PB5
#define LED_DDR DDRB
#define LED_PORT PORTB

enum pitch {
#ifndef DISABLE_UNCOMMON_PITCHES
    C3,
    CS3,
    D3,
    DS3,
    E3,
    F3,
    FS3,
    G3,
    GS3,
    A3,
    AS3,
    B3,
#endif

    C4,
    CS4,
    D4,
    DS4,
    E4,
    F4,
    FS4,
    G4,
    GS4,
    A4,
    AS4,
    B4,

    C5,
    CS5,
    D5,
#ifndef DISABLE_UNCOMMON_PITCHES
    DS5,
    E5,
    F5,
    FS5,
    G5,
    GS5,
    A5,
    AS5,
    B5,
#endif
};

#ifndef DISABLE_UNCOMMON_PITCHES
#define C3_FREQUENCY 130813
#define CS3_FREQUENCY 138591
#define D3_FREQUENCY 146832
#define DS3_FREQUENCY 155564
#define E3_FREQUENCY 164814
#define F3_FREQUENCY 174614
#define FS3_FREQUENCY 184997
#define G3_FREQUENCY 195998
#define GS3_FREQUENCY 207652
#define A3_FREQUENCY 220000
#define AS3_FREQUENCY 233082
#define B3_FREQUENCY 246942
#endif

#define C4_FREQUENCY 261626
#define CS4_FREQUENCY 277183
#define D4_FREQUENCY 293665
#define DS4_FREQUENCY 311127
#define E4_FREQUENCY 329628
#define F4_FREQUENCY 349228
#define FS4_FREQUENCY 369994
#define G4_FREQUENCY 391995
#define GS4_FREQUENCY 415305
#define A4_FREQUENCY 440000
#define AS4_FREQUENCY 466164
#define B4_FREQUENCY 493883

#define C5_FREQUENCY 523251
#define CS5_FREQUENCY 554365
#define D5_FREQUENCY 587329
#ifndef DISABLE_UNCOMMON_PITCHES
#define DS5_FREQUENCY 622254
#define E5_FREQUENCY 659255
#define F5_FREQUENCY 698457
#define FS5_FREQUENCY 739989
#define G5_FREQUENCY 783991
#define GS5_FREQUENCY 830609
#define A5_FREQUENCY 880000
#define AS5_FREQUENCY 932328
#define B5_FREQUENCY 987767
#endif

static const uint32_t pitch_to_frequency[] = {
#ifndef DISABLE_UNCOMMON_PITCHES
    C3_FREQUENCY,
    CS3_FREQUENCY,
    D3_FREQUENCY,
    DS3_FREQUENCY,
    E3_FREQUENCY,
    F3_FREQUENCY,
    FS3_FREQUENCY,
    G3_FREQUENCY,
    GS3_FREQUENCY,
    A3_FREQUENCY,
    AS3_FREQUENCY,
    B3_FREQUENCY,
#endif

    C4_FREQUENCY,
    CS4_FREQUENCY,
    D4_FREQUENCY,
    DS4_FREQUENCY,
    E4_FREQUENCY,
    F4_FREQUENCY,
    FS4_FREQUENCY,
    G4_FREQUENCY,
    GS4_FREQUENCY,
    A4_FREQUENCY,
    AS4_FREQUENCY,
    B4_FREQUENCY,

    C5_FREQUENCY,
    CS5_FREQUENCY,
    D5_FREQUENCY,
#ifndef DISABLE_UNCOMMON_PITCHES
    DS5_FREQUENCY,
    E5_FREQUENCY,
    F5_FREQUENCY,
    FS5_FREQUENCY,
    G5_FREQUENCY,
    GS5_FREQUENCY,
    A5_FREQUENCY,
    AS5_FREQUENCY,
    B5_FREQUENCY,
#endif
};

#define DELAY_US(pitch) _delay_us(1000000000 / (pitch) / 2)

static void pitch_as_delay(enum pitch pitch) {
    switch (pitch) {
#ifndef DISABLE_UNCOMMON_PITCHES
    case C3:
        DELAY_US(C3_FREQUENCY);
        break;
    case CS3:
        DELAY_US(CS3_FREQUENCY);
        break;
    case D3:
        DELAY_US(D3_FREQUENCY);
        break;
    case DS3:
        DELAY_US(DS3_FREQUENCY);
        break;
    case E3:
        DELAY_US(E3_FREQUENCY);
        break;
    case F3:
        DELAY_US(F3_FREQUENCY);
        break;
    case FS3:
        DELAY_US(FS3_FREQUENCY);
        break;
    case G3:
        DELAY_US(G3_FREQUENCY);
        break;
    case GS3:
        DELAY_US(GS3_FREQUENCY);
        break;
    case A3:
        DELAY_US(A3_FREQUENCY);
        break;
    case AS3:
        DELAY_US(AS3_FREQUENCY);
        break;
    case B3:
        DELAY_US(B3_FREQUENCY);
        break;
#endif

    case C4:
        DELAY_US(C4_FREQUENCY);
        break;
    case CS4:
        DELAY_US(CS4_FREQUENCY);
        break;
    case D4:
        DELAY_US(D4_FREQUENCY);
        break;
    case DS4:
        DELAY_US(DS4_FREQUENCY);
        break;
    case E4:
        DELAY_US(E4_FREQUENCY);
        break;
    case F4:
        DELAY_US(F4_FREQUENCY);
        break;
    case FS4:
        DELAY_US(FS4_FREQUENCY);
        break;
    case G4:
        DELAY_US(G4_FREQUENCY);
        break;
    case GS4:
        DELAY_US(GS4_FREQUENCY);
        break;
    case A4:
        DELAY_US(A4_FREQUENCY);
        break;
    case AS4:
        DELAY_US(AS4_FREQUENCY);
        break;
    case B4:
        DELAY_US(B4_FREQUENCY);
        break;

    case C5:
        DELAY_US(C5_FREQUENCY);
        break;
    case CS5:
        DELAY_US(CS5_FREQUENCY);
        break;
    case D5:
        DELAY_US(D5_FREQUENCY);
        break;
#ifndef DISABLE_UNCOMMON_PITCHES
    case DS5:
        DELAY_US(DS5_FREQUENCY);
        break;
    case E5:
        DELAY_US(E5_FREQUENCY);
        break;
    case F5:
        DELAY_US(F5_FREQUENCY);
        break;
    case FS5:
        DELAY_US(FS5_FREQUENCY);
        break;
    case G5:
        DELAY_US(G5_FREQUENCY);
        break;
    case GS5:
        DELAY_US(GS5_FREQUENCY);
        break;
    case A5:
        DELAY_US(A5_FREQUENCY);
        break;
    case AS5:
        DELAY_US(AS5_FREQUENCY);
        break;
    case B5:
        DELAY_US(B5_FREQUENCY);
        break;
#endif

    default:
        // nieprawidłowa wysokość dźwięku
        LED_PORT |= _BV(LED);
        _delay_ms(1000);
        LED_PORT &= ~_BV(LED);
        break;
    }
}

enum duration {
    WHOLE,
    WHOLE_DOT,
    HALF,
    HALF_DOT,
    QUARTER,
    QUARTER_DOT,
    EIGHT,
    EIGHT_DOT,
    SIXTEENTH,
    SIXTEENTH_DOT,
    THIRTY_SECOND,
    THIRTY_SECOND_DOT,
    SIXTY_FOURTH,
    SIXTY_FOURTH_DOT,
    HUNDRED_TWENTY_EIGHTH,
    HUNDRED_TWENTY_EIGHTH_DOT,
};

// #define TEMPO 2000
// #define TEMPO 1700
#define TEMPO 2000

#define WHOLE_LENGTH TEMPO
#define WHOLE_DOT_LENGTH WHOLE_LENGTH + WHOLE_LENGTH / 2

#define HALF_LENGTH WHOLE_LENGTH / 2
#define HALF_DOT_LENGTH HALF_LENGTH + HALF_LENGTH / 2

#define QUARTER_LENGTH HALF_LENGTH / 2
#define QUARTER_DOT_LENGTH QUARTER_LENGTH + QUARTER_LENGTH / 2

#define EIGHT_LENGTH QUARTER_LENGTH / 2
#define EIGHT_DOT_LENGTH EIGHT_LENGTH + EIGHT_LENGTH / 2

#define SIXTEENTH_LENGTH EIGHT_LENGTH / 2
#define SIXTEENTH_DOT_LENGTH SIXTEENTH_LENGTH + SIXTEENTH_LENGTH / 2

#define THIRTY_SECOND_LENGTH SIXTEENTH_LENGTH / 2
#define THIRTY_SECOND_DOT_LENGTH THIRTY_SECOND_LENGTH + THIRTY_SECOND_LENGTH / 2

#define SIXTY_FOURTH_LENGTH THIRTY_SECOND_LENGTH / 2
#define SIXTY_FOURTH_DOT_LENGTH SIXTY_FOURTH_LENGTH + SIXTY_FOURTH_LENGTH / 2

#define HUNDRED_TWENTY_EIGHTH_LENGTH SIXTY_FOURTH_LENGTH / 2
#define HUNDRED_TWENTY_EIGHTH_DOT_LENGTH HUNDRED_TWENTY_EIGHTH_LENGTH + HUNDRED_TWENTY_EIGHTH_LENGTH / 2

static const uint16_t duration_to_length[] = {
    WHOLE_LENGTH,
    WHOLE_DOT_LENGTH,
    HALF_LENGTH,
    HALF_DOT_LENGTH,
    QUARTER_LENGTH,
    QUARTER_DOT_LENGTH,
    EIGHT_LENGTH,
    EIGHT_DOT_LENGTH,
    SIXTEENTH_LENGTH,
    SIXTEENTH_DOT_LENGTH,
    THIRTY_SECOND_LENGTH,
    THIRTY_SECOND_DOT_LENGTH,
    SIXTY_FOURTH_LENGTH,
    SIXTY_FOURTH_DOT_LENGTH,
    HUNDRED_TWENTY_EIGHTH_LENGTH,
    HUNDRED_TWENTY_EIGHTH_DOT_LENGTH,
};

static void duration_as_delay(enum duration duration) {
    switch (duration) {
    case WHOLE:
        _delay_ms(WHOLE_LENGTH);
        break;
    case WHOLE_DOT:
        _delay_ms(WHOLE_DOT_LENGTH);
        break;
    case HALF:
        _delay_ms(HALF_LENGTH);
        break;
    case HALF_DOT:
        _delay_ms(HALF_DOT_LENGTH);
        break;
    case QUARTER:
        _delay_ms(QUARTER_LENGTH);
        break;
    case QUARTER_DOT:
        _delay_ms(QUARTER_DOT_LENGTH);
        break;
    case EIGHT:
        _delay_ms(EIGHT_LENGTH);
        break;
    case EIGHT_DOT:
        _delay_ms(EIGHT_DOT_LENGTH);
        break;
    case SIXTEENTH:
        _delay_ms(SIXTEENTH_LENGTH);
        break;
    case SIXTEENTH_DOT:
        _delay_ms(SIXTEENTH_DOT_LENGTH);
        break;
    case THIRTY_SECOND:
        _delay_ms(THIRTY_SECOND_LENGTH);
        break;
    case THIRTY_SECOND_DOT:
        _delay_ms(THIRTY_SECOND_DOT_LENGTH);
        break;
    case SIXTY_FOURTH:
        _delay_ms(SIXTY_FOURTH_LENGTH);
        break;
    case SIXTY_FOURTH_DOT:
        _delay_ms(SIXTY_FOURTH_DOT_LENGTH);
        break;
    case HUNDRED_TWENTY_EIGHTH:
        _delay_ms(HUNDRED_TWENTY_EIGHTH_LENGTH);
        break;
    case HUNDRED_TWENTY_EIGHTH_DOT:
        _delay_ms(HUNDRED_TWENTY_EIGHTH_DOT_LENGTH);
        break;

    default:
        // nieprawidłowy czas trwania dźwięku
        LED_PORT |= _BV(LED);
        _delay_ms(333);
        LED_PORT &= ~_BV(LED);
        _delay_ms(333);
        LED_PORT |= _BV(LED);
        _delay_ms(333);
        LED_PORT &= ~_BV(LED);
        break;
    }
}

inline static void play_note(enum pitch pitch, enum duration duration) {
    uint32_t frequency = pitch_to_frequency[pitch];
    uint16_t length = duration_to_length[duration];
    for (uint32_t index = 0; index < (uint32_t)frequency * length / 1000000; index++) {
        BUZZ_PORT |= _BV(BUZZ);
        pitch_as_delay(pitch);
        BUZZ_PORT &= ~_BV(BUZZ);
        pitch_as_delay(pitch);
    }
}

inline static void play_rest(enum duration duration) {
    duration_as_delay(duration);
}

#ifdef DISABLE_UNCOMMON_PITCHES
// kodowanie 15 wysokości + pauza i 16 długości
#define NOTE(pitch, duration) ((pitch << 4) | duration),
#define REST_PITCH 0xF
#define REST(duration) ((REST_PITCH << 4) | duration),
#define DOUBLE(items) items items
#else
// kodowanie 255 wysokości + pauza i 256 długości
#define NOTE(pitch, duration) pitch, duration,
#define REST_PITCH 0xFF
#define REST(duration) REST_PITCH, duration,
#define DOUBLE(items) items items
#endif

static const uint8_t melody[] PROGMEM = {
    // @formatter:off
    // https://s3.amazonaws.com/halleonard-pagepreviews/HL_DDS_912405L1wq5Llho6.png
    // https://www.youtube.com/watch?v=wQ5uaLC_13M
    NOTE(C4, EIGHT)
    NOTE(A3, EIGHT)
    NOTE(G3, EIGHT)
    NOTE(C3, EIGHT)

    NOTE(E3, QUARTER_DOT)
    NOTE(DS3, EIGHT_DOT)
    /// REST(HUNDRED_TWENTY_EIGHTH_DOT)
    NOTE(DS3, EIGHT)
    // REST(HUNDRED_TWENTY_EIGHTH_DOT)
    NOTE(DS3, QUARTER)
    NOTE(F3,  EIGHT)
    /// REST(HUNDRED_TWENTY_EIGHTH_DOT)
    NOTE(F3,  EIGHT)
    // REST(HUNDRED_TWENTY_EIGHTH_DOT)
    NOTE(F3,  QUARTER)

    NOTE(G3,  EIGHT)
    /// REST(HUNDRED_TWENTY_EIGHTH_DOT)
    NOTE(G3,  EIGHT)
    // REST(HUNDRED_TWENTY_EIGHTH_DOT)
    NOTE(G3,  QUARTER)
    NOTE(E3,  EIGHT_DOT)

    NOTE(C4,  EIGHT)
    NOTE(A3,  EIGHT)
    NOTE(C4,  EIGHT)
    NOTE(C3,  EIGHT)

    NOTE(E3, QUARTER_DOT)
    NOTE(DS3, EIGHT)
    /// REST(HUNDRED_TWENTY_EIGHTH_DOT)
    NOTE(DS3, EIGHT)
    // REST(HUNDRED_TWENTY_EIGHTH_DOT)
    NOTE(DS3, QUARTER)
    NOTE(F3, EIGHT)

    /// REST(HUNDRED_TWENTY_EIGHTH_DOT)
    NOTE(F3, EIGHT)
    // REST(HUNDRED_TWENTY_EIGHTH_DOT)
    NOTE(F3, QUARTER)
    NOTE(G3, EIGHT)
    /// REST(HUNDRED_TWENTY_EIGHTH_DOT)
    NOTE(G3, EIGHT)
    // REST(HUNDRED_TWENTY_EIGHTH_DOT)
    NOTE(G3, QUARTER)
    NOTE(DS3, EIGHT)

    NOTE(C4, EIGHT)
    NOTE(A3, EIGHT)
    NOTE(G3, EIGHT)
    NOTE(A3, EIGHT)
    NOTE(C3, EIGHT)

    NOTE(E3, QUARTER_DOT)
    NOTE(DS3, EIGHT)
    /// REST(HUNDRED_TWENTY_EIGHTH_DOT)
    NOTE(DS3, EIGHT)
    // REST(HUNDRED_TWENTY_EIGHTH_DOT)
    NOTE(DS3, QUARTER)
    NOTE(F3, EIGHT)

    /// REST(HUNDRED_TWENTY_EIGHTH_DOT)
    NOTE(F3, EIGHT)
    // REST(HUNDRED_TWENTY_EIGHTH_DOT)
    NOTE(F3, QUARTER)
    NOTE(G3, EIGHT)
    /// REST(HUNDRED_TWENTY_EIGHTH_DOT)
    NOTE(G3, EIGHT)
    // REST(HUNDRED_TWENTY_EIGHTH_DOT)
    NOTE(G3, QUARTER)
    NOTE(DS3, EIGHT)

    NOTE(C4, EIGHT)
    NOTE(A3, EIGHT)
    NOTE(G3, EIGHT)
    NOTE(C3, EIGHT)

    NOTE(C4, QUARTER_DOT)
    NOTE(C4, EIGHT)
    NOTE(C4, QUARTER)
    NOTE(E3, QUARTER)
    NOTE(C4, EIGHT)
    NOTE(C4, QUARTER_DOT)
    NOTE(F3, QUARTER)
    NOTE(C4, EIGHT)
    NOTE(C4, QUARTER_DOT)
    NOTE(G3, QUARTER)
    NOTE(F4, EIGHT)
    NOTE(C4, EIGHT)
    NOTE(F4, QUARTER)

    // https://www.youtube.com/watch?v=vRFWPwCaaYs
    DOUBLE(
        DOUBLE(
            NOTE(A4, QUARTER)
            REST(QUARTER)
            NOTE(A4, EIGHT)
            REST(HUNDRED_TWENTY_EIGHTH)
            NOTE(A4, SIXTEENTH)
            REST(HUNDRED_TWENTY_EIGHTH_DOT)
            NOTE(A4, SIXTEENTH)
            NOTE(G4, SIXTEENTH)
            NOTE(A4, SIXTEENTH)
            REST(EIGHT))
        NOTE(A4, QUARTER)
        REST(EIGHT)
        NOTE(C5, QUARTER)
        NOTE(A4, QUARTER)
        NOTE(G4, QUARTER)
        NOTE(F4, QUARTER)
        NOTE(D4, EIGHT)
        REST(HUNDRED_TWENTY_EIGHTH)
        NOTE(D4, EIGHT)
        NOTE(E4, EIGHT)
        NOTE(F4, EIGHT)
        NOTE(D4, EIGHT))
    // https://www.youtube.com/watch?v=RlJpBDGS890
    DOUBLE(
        NOTE(AS4, QUARTER)
        REST(QUARTER)
        DOUBLE(
            NOTE(AS4, EIGHT)
            REST(THIRTY_SECOND)
            NOTE(AS4, SIXTEENTH)
            REST(HUNDRED_TWENTY_EIGHTH)
            NOTE(AS4, SIXTEENTH)
            NOTE(GS4, SIXTEENTH)
            NOTE(AS4, EIGHT)
            REST(HUNDRED_TWENTY_EIGHTH)
            NOTE(AS4, QUARTER)
            REST(EIGHT_DOT))
        NOTE(CS5, QUARTER)
        NOTE(AS4, EIGHT)
        REST(EIGHT)
        NOTE(GS4, QUARTER)
        NOTE(FS4, EIGHT)
        REST(EIGHT)
        NOTE(DS4, EIGHT)
        REST(THIRTY_SECOND)
        NOTE(DS4, EIGHT)
        NOTE(F4, EIGHT)
        NOTE(FS4, EIGHT)
        NOTE(DS4, EIGHT))
    // @formatter:on
};
#ifdef DISABLE_UNCOMMON_PITCHES
#define MELODY_SIZE sizeof(melody)
#else
#define MELODY_SIZE sizeof(melody) / 2
#endif

int main() {
    BUZZ_DDR |= _BV(BUZZ);
    LED_DDR |= _BV(LED);

    while (1) {
        for (uint8_t index = 0; index < MELODY_SIZE; index++) {
#ifdef DISABLE_UNCOMMON_PITCHES
            uint8_t note = pgm_read_byte(&melody[index]);
            enum pitch pitch = (note & 0xF0) >> 4;
            enum duration duration = note & 0x0F;
#else
            enum pitch pitch = pgm_read_byte(&melody[2 * index]);
            enum duration duration = pgm_read_byte(&melody[2 * index + 1]);
#endif
            if (pitch == REST_PITCH) {
                play_rest(duration);
            } else {
                play_note(pitch, duration);
            }
        }
    }
}
