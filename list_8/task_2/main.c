#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include <assert.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <stdio.h>

#define BAUD 9600 // baudrate
#define UBRR_VALUE ((F_CPU) / 16 / (BAUD)-1) // zgodnie ze wzorem

// inicjalizacja UART
static void uart_init(void) {
    // ustaw baudrate
    UBRR0 = UBRR_VALUE;
    // wyczyść rejestr UCSR0A
    UCSR0A = 0;
    // włącz odbiornik i nadajnik
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);
    // ustaw format 8n1
    UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);
}

// transmisja jednego znaku
int uart_transmit(char data, FILE* stream) {
    // czekaj aż transmiter gotowy
    while (!(UCSR0A & _BV(UDRE0))) {
        taskYIELD();
    }
    UDR0 = data;
    return 0;
}

// odczyt jednego znaku
int uart_receive(FILE* stream) {
    // czekaj aż znak dostępny
    while (!(UCSR0A & _BV(RXC0))) {
        taskYIELD();
    }
    return UDR0;
}

FILE uart_file;

#define DEBUG
#undef DEBUG

#ifdef DEBUG
#define DEBUG_LED PB5
#define DEBUG_LED_DDR DDRB
#define DEBUG_LED_PORT PORTB
#define DEBUG_VALUE 0b01010101
static volatile StackType_t* debug_address;
#endif

#define NUMBER_TYPE uint16_t

#define NUMBER_READER_TASK_STACK_SIZE configMINIMAL_STACK_SIZE + 200
#define NUMBER_READER_TASK_PRIORITY 1

static void number_reader_task(void* parameters) {
    QueueHandle_t queue = *(QueueHandle_t*)parameters;

    while (1) {
        printf("Podaj przez jaki czas ma się świecić dioda...\r\n");
        NUMBER_TYPE number;
        scanf("%" SCNu16, &number);
        xQueueSend(queue, &number, 0);
    }
}

#define LED_PORT PORTC
#define LED_DDR DDRC
#define LED PC5

#define LED_DELAY 500

#define LED_TASK_STACK_SIZE configMINIMAL_STACK_SIZE + 180
#define LED_TASK_PRIORITY 2

static void led_task(void* parameters) {
    QueueHandle_t queue = *(QueueHandle_t*)parameters;

    LED_DDR |= _BV(LED);
    LED_PORT &= ~_BV(LED);

    while (1) {
        NUMBER_TYPE number;
        BaseType_t queue_result = xQueueReceive(queue, &number, 10);
        if (queue_result == pdPASS) {
            LED_PORT |= _BV(LED);
            TickType_t previous_wake_time = xTaskGetTickCount();
            vTaskDelayUntil(&previous_wake_time, number);
            LED_PORT &= ~_BV(LED);
            vTaskDelayUntil(&previous_wake_time, LED_DELAY);
        }
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
    static StaticTask_t name##_task_buffer;                                 \
    xTaskHandle name##_task_handle = xTaskCreateStatic(                     \
        handler, #name, stack_size, parameters, priority,                   \
        name##_task_stack, &name##_task_buffer);                            \
    assert(name##_task_handle != NULL);

#define QUEUE_SIZE 32
#define QUEUE_ITEM_SIZE sizeof(NUMBER_TYPE)

int main(void) {
    // zainicjalizuj UART
    uart_init();
    // skonfiguruj strumienie wejścia/wyjścia
    fdev_setup_stream(&uart_file, uart_transmit, uart_receive, _FDEV_SETUP_RW);
    stdin = stdout = stderr = &uart_file;

    set_sleep_mode(SLEEP_MODE_IDLE);

    uint8_t queue_storage[QUEUE_SIZE * QUEUE_ITEM_SIZE];
    static StaticQueue_t static_queue;
    QueueHandle_t queue = xQueueCreateStatic(QUEUE_SIZE, QUEUE_ITEM_SIZE, queue_storage, &static_queue);
    assert(queue != NULL);

    CREATE_STATIC_TASK(
        number_reader_task, numread,
        NUMBER_READER_TASK_STACK_SIZE, &queue,
        NUMBER_READER_TASK_PRIORITY);

    CREATE_STATIC_TASK(
        led_task, led,
        LED_TASK_STACK_SIZE, &queue, LED_TASK_PRIORITY);

#ifdef DEBUG
    DEBUG_LED_DDR |= _BV(DEBUG_LED);
    debug_address = number_reader_task;
    *debug_address = DEBUG_VALUE;
#endif

    vTaskStartScheduler();
    return 0;
}
