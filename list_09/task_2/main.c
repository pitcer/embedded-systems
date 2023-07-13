#include "hd44780.h"
#include <avr/io.h>
#include <inttypes.h>
#include <stdio.h>
#include <util/delay.h>

#define LCD_WIDTH 16
#define BLOCK_WIDTH 5
#define BLOCK_HEIGHT 8
#define MAXIMUM_PROGRESS ((LCD_WIDTH) * (BLOCK_WIDTH))

#define FIRST_BLOCK_CHARACTER '\x00'
#define EMPTY_BLOCK_CHARACTER FIRST_BLOCK_CHARACTER
#define FULL_BLOCK_CHARACTER (EMPTY_BLOCK_CHARACTER + BLOCK_WIDTH)

static int hd44780_transmit(char data, FILE* stream) {
    LCD_WriteData(data);
    return 0;
}

static FILE lcd_file;

static void lcd_select_character(uint8_t address) {
    LCD_WriteCommand(HD44780_CGRAM_SET | (0x3f & (address << 3)));
}

static void write_column_character(uint8_t width) {
    const uint8_t block_mask = 0x3f & -_BV(5 - width);
    for (uint8_t index = 0; index < BLOCK_HEIGHT; index++) {
        LCD_WriteData(block_mask);
    }
}

static inline void initialize_block_characters(void) {
    lcd_select_character(EMPTY_BLOCK_CHARACTER);
    for (uint8_t width = 0; width <= BLOCK_WIDTH; width++) {
        write_column_character(width);
    }
}

static void clear_line(uint8_t line) {
    LCD_GoTo(0, line);
    for (uint8_t index = 0; index < LCD_WIDTH; index++) {
        putchar(EMPTY_BLOCK_CHARACTER);
    }
}

static inline void put_progress_character(uint8_t progress) {
    const uint8_t full_blocks = progress / BLOCK_WIDTH;
    const uint8_t last_block_fill = progress % BLOCK_WIDTH;

    if (last_block_fill == 0) {
        if (progress > 0) {
            LCD_GoTo(full_blocks - 1, 0);
            putchar(FULL_BLOCK_CHARACTER);
        }
        return;
    }

    LCD_GoTo(full_blocks, 0);
    char last_block = EMPTY_BLOCK_CHARACTER + last_block_fill;
    putchar(last_block);
}

int main(void) {
    LCD_Initialize();
    LCD_Clear();

    fdev_setup_stream(&lcd_file, hd44780_transmit, NULL, _FDEV_SETUP_RW);
    stdout = stderr = &lcd_file;

    initialize_block_characters();

    LCD_GoTo(2, 1);
    printf("/%" PRIu8, MAXIMUM_PROGRESS);
    for (char block = FIRST_BLOCK_CHARACTER; block <= FULL_BLOCK_CHARACTER; block++) {
        putchar(block);
    }

    clear_line(0);
    uint8_t progress = 0;
    while (1) {
        put_progress_character(progress);

        LCD_GoTo(0, 1);
        printf("%.2" PRIu8, progress);

        _delay_ms(250);
        progress++;
        if (progress == MAXIMUM_PROGRESS + 1) {
            clear_line(0);
            progress = 0;
        }
    }
}
