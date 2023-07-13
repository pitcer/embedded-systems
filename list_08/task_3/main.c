#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include <assert.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <util/delay.h>

#define DEBUG
#undef DEBUG

#ifdef DEBUG
#define DEBUG_LED PB5
#define DEBUG_LED_DDR DDRB
#define DEBUG_LED_PORT PORTB
#define DEBUG_VALUE 0b01010101
static volatile StackType_t* debug_address;
#endif

#define IO_TASK_STACK_SIZE configMINIMAL_STACK_SIZE + 48
#define IO_TASK_PRIORITY 2

static void io_task(void* parameters) {
    (void)parameters;

    while (1) {
        printf("Podaj liczbę...\r\n");
        uint16_t number;
        scanf("%" SCNu16, &number);
        printf("Liczba: %" PRIu16 "\r\n", number);
        for (uint8_t index = 0; index < 32; index++) {
            printf("Foobar\r\n");
        }
    }
}

#define LED_PORT PORTC
#define LED_DDR DDRC
#define LED PC5

#define LED_DELAY 250

#define BLINKING_LED_TASK_STACK_SIZE configMINIMAL_STACK_SIZE + 32
#define BLINKING_LED_TASK_PRIORITY 1

static void blinking_led_task(void* parameters) {
    (void)parameters;

    LED_DDR |= _BV(LED);
    LED_PORT &= ~_BV(LED);

    TickType_t previous_wake_time = xTaskGetTickCount();
    while (1) {
        LED_PORT |= _BV(LED);
        vTaskDelayUntil(&previous_wake_time, LED_DELAY);
        LED_PORT &= ~_BV(LED);
        vTaskDelayUntil(&previous_wake_time, LED_DELAY);
    }
}

#define BAUD 9600 // baudrate
#define UBRR_VALUE ((F_CPU) / 16 / (BAUD)-1) // zgodnie ze wzorem

// inicjalizacja UART
static void initialize_uart(void) {
    // ustaw baudrate
    UBRR0 = UBRR_VALUE;
    // wyczyść USART Data Register Empty
    UCSR0A &= ~_BV(UDRE0);
    UCSR0B |= _BV(RXCIE0); // receive complete interrupt enable
    UCSR0B |= _BV(RXEN0); // enable receiver
    UCSR0B |= _BV(TXEN0); // enable transmitter
    UCSR0C = _BV(UCSZ00) | _BV(UCSZ01); // character size 8 bits; set by default
}

#define QUEUE_SIZE 128
#define QUEUE_ITEM uint8_t
#define QUEUE_ITEM_SIZE sizeof(QUEUE_ITEM)

static volatile QueueHandle_t receiver_queue;
static volatile QueueHandle_t transmitter_queue;

#define ENABLE_DATA_REGISTER_EMPTY_INTERRUPT UCSR0B |= _BV(UDRIE0)
#define DISABLE_DATA_REGISTER_EMPTY_INTERRUPT UCSR0B &= ~_BV(UDRIE0)

// Receive Complete Interrupt
ISR(USART_RX_vect) {
    if (!xQueueIsQueueFullFromISR(receiver_queue)) {
        uint8_t data = UDR0;
        xQueueSendFromISR(receiver_queue, &data, NULL);
    }
}

// Data Register Empty Interrupt
ISR(USART_UDRE_vect) {
    uint8_t data;
    BaseType_t result = xQueueReceiveFromISR(transmitter_queue, &data, NULL);
    if (result) {
        UDR0 = data;
    } else {
        DISABLE_DATA_REGISTER_EMPTY_INTERRUPT;
    }
}

static int uart_transmit(char data, FILE* stream) {
    // https://freertos.org/a00117.html
    xQueueSend(transmitter_queue, &data, portMAX_DELAY);
    ENABLE_DATA_REGISTER_EMPTY_INTERRUPT;
    return 0;
}

static int uart_receive(FILE* stream) {
    QUEUE_ITEM result;
    // https://freertos.org/a00118.html
    xQueueReceive(receiver_queue, &result, portMAX_DELAY);
    return result;
}

FILE uart_file;

void vApplicationIdleHook(void) {
#ifdef DEBUG
    if (*debug_address != DEBUG_VALUE) {
        DEBUG_LED_PORT |= _BV(DEBUG_LED);
    }
#endif
    sleep_mode();
#ifdef DEBUG
    DEBUG_LED_PORT &= ~_BV(DEBUG_LED);
#endif
}

#define IDLE_TASK_STACK_SIZE configMINIMAL_STACK_SIZE + (2 * QUEUE_SIZE * QUEUE_ITEM_SIZE)

// https://freertos.org/a00110.html#configSUPPORT_STATIC_ALLOCATION
void vApplicationGetIdleTaskMemory(
    StaticTask_t** ppxIdleTaskTCBBuffer,
    StackType_t** ppxIdleTaskStackBuffer,
    uint32_t* pulIdleTaskStackSize) {

    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[IDLE_TASK_STACK_SIZE];

    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = IDLE_TASK_STACK_SIZE;
}

#define CREATE_STATIC_TASK(handler, name, stack_size, parameters, priority) \
    StackType_t name##_task_stack[stack_size];                              \
    static StaticTask_t name##_task_buffer;                                 \
    xTaskHandle name##_task_handle = xTaskCreateStatic(                     \
        handler, #name, stack_size, parameters, priority,                   \
        name##_task_stack, &name##_task_buffer);                            \
    assert(name##_task_handle != NULL);

#define CREATE_STATIC_QUEUE(name, size, item_size)         \
    uint8_t name##_queue_storage[(size) * (item_size)];    \
    static StaticQueue_t name##_static_queue;              \
    name##_queue = xQueueCreateStatic((size), (item_size), \
        name##_queue_storage, &name##_static_queue);       \
    assert(name##_queue != NULL);

int main(void) {
    CREATE_STATIC_QUEUE(receiver, QUEUE_SIZE, QUEUE_ITEM_SIZE);
    CREATE_STATIC_QUEUE(transmitter, QUEUE_SIZE, QUEUE_ITEM_SIZE);

    initialize_uart();

    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    set_sleep_mode(SLEEP_MODE_IDLE);

    CREATE_STATIC_TASK(io_task, io, IO_TASK_STACK_SIZE, NULL, IO_TASK_PRIORITY);

    CREATE_STATIC_TASK(blinking_led_task, blnkled, BLINKING_LED_TASK_STACK_SIZE,
        NULL, BLINKING_LED_TASK_PRIORITY);

#ifdef DEBUG
    DEBUG_LED_DDR |= _BV(DEBUG_LED);
    debug_address = io_task_stack;
    *debug_address = DEBUG_VALUE;
#endif

    sei();

    vTaskStartScheduler();
    return 0;
}
