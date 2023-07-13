#include "FreeRTOS.h"
#include "task.h"
#include <assert.h>
#include <avr/io.h>
#include <avr/sleep.h>

#define DEBUG
#undef DEBUG

#ifdef DEBUG
#define DEBUG_LED PB5
#define DEBUG_LED_DDR DDRB
#define DEBUG_LED_PORT PORTB
#define DEBUG_VALUE 0b01010101
static volatile StackType_t* debug_address;
#endif

#define CYLON_EYE_DELAY 100

#define CYLON_EYE_TASK_STACK_SIZE configMINIMAL_STACK_SIZE + 2
#define CYLON_EYE_TASK_PRIORITY 1

static void cylon_eye_task(void* parameters) {
    (void)parameters;

    UCSR0B &= ~_BV(RXEN0) & ~_BV(TXEN0);

    DDRD |= 0xFF;

    uint8_t index = 0;

    TickType_t previous_wake_rime = xTaskGetTickCount();

    while (1) {
        for (; index < 7; index++) {
            PORTD = _BV(index);
            vTaskDelayUntil(&previous_wake_rime, CYLON_EYE_DELAY);
        }

        for (; index > 0; index--) {
            PORTD = _BV(index);
            vTaskDelayUntil(&previous_wake_rime, CYLON_EYE_DELAY);
        }
    }
}

#define BUTTON_PORT PORTC
#define BUTTON_PIN PINC
#define BUTTON PC4

#define LED_PORT PORTC
#define LED_DDR DDRC
#define LED PC5

#define LED_BUFFER_SIZE 100
#define LED_DELAY 10

#define MEMORIZING_LED_TASK_STACK_SIZE configMINIMAL_STACK_SIZE + 2 + LED_BUFFER_SIZE
#define MEMORIZING_LED_TASK_PRIORITY 1

static void memorizing_led_task(void* parameters) {
    (void)parameters;

    LED_DDR |= _BV(LED);
    LED_PORT &= ~_BV(LED);

    BUTTON_PORT |= _BV(BUTTON);

    uint8_t buffer[LED_BUFFER_SIZE] = { 0 };
    uint8_t write_position = 0;

    TickType_t previous_wake_rime = xTaskGetTickCount();

    while (1) {
        buffer[write_position] = !((BUTTON_PIN & _BV(BUTTON)) >> BUTTON);
        write_position = (write_position + 1) % LED_BUFFER_SIZE;
        LED_PORT = (LED_PORT & ~_BV(LED)) | buffer[write_position] << LED;
        vTaskDelayUntil(&previous_wake_rime, LED_DELAY);
    }
}

void vApplicationIdleHook(void) {
#ifdef DEBUG
    if (*debug_address != DEBUG_VALUE) {
        DEBUG_LED_PORT |= _BV(DEBUG_LED);
    }
#endif
    sleep_mode();
}

// https://freertos.org/a00110.html#configSUPPORT_STATIC_ALLOCATION
void vApplicationGetIdleTaskMemory(
    StaticTask_t** ppxIdleTaskTCBBuffer,
    StackType_t** ppxIdleTaskStackBuffer,
    uint32_t* pulIdleTaskStackSize) {

    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

#define CREATE_STATIC_TASK(handler, name, stack_size, parameters, priority) \
    StackType_t name##_task_stack[stack_size];                              \
    StaticTask_t name##_task_buffer;                                        \
    xTaskHandle name##_task_handle = xTaskCreateStatic(                     \
        handler, #name, stack_size, parameters, priority,                   \
        name##_task_stack, &name##_task_buffer);                            \
    assert(name##_task_handle != NULL);

int main(void) {
    set_sleep_mode(SLEEP_MODE_IDLE);

    CREATE_STATIC_TASK(
        cylon_eye_task, cyloeye,
        CYLON_EYE_TASK_STACK_SIZE, NULL, CYLON_EYE_TASK_PRIORITY);

    CREATE_STATIC_TASK(
        memorizing_led_task, memoled,
        MEMORIZING_LED_TASK_STACK_SIZE, NULL, MEMORIZING_LED_TASK_PRIORITY);

#ifdef DEBUG
    DEBUG_LED_DDR |= _BV(DEBUG_LED);
    debug_address = memoled_task_stack;
    *debug_address = DEBUG_VALUE;
#endif

    vTaskStartScheduler();
    return 0;
}
