#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include <assert.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <stdio.h>

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

#define QUEUE_SIZE 64
#define QUEUE_ITEM uint8_t
#define QUEUE_ITEM_SIZE sizeof(QUEUE_ITEM)

QueueHandle_t receiver_queue;
QueueHandle_t transmitter_queue;

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
    xQueueSend(transmitter_queue, &data, portMAX_DELAY);
    ENABLE_DATA_REGISTER_EMPTY_INTERRUPT;
    return 0;
}

static int uart_receive(FILE* stream) {
    QUEUE_ITEM result;
    xQueueReceive(receiver_queue, &result, portMAX_DELAY);
    return result;
}

FILE uart_file;

#define POTENTIOMETER_MUX ADC0D
#define THERMISTOR_MUX ADC1D
#define PHOTORESISTOR_MUX ADC2D

static void initialize_adc(void) {
    ADMUX = _BV(REFS0); // referencja AVcc = 5V
    // wyłącz wejścia cyfrowe na ADC0|1|2
    DIDR0 = _BV(POTENTIOMETER_MUX) | _BV(THERMISTOR_MUX) | _BV(PHOTORESISTOR_MUX);
    // częstotliwość zegara ADC 125 kHz (16 MHz / 128)
    ADCSRA = _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); // preskaler 128
    ADCSRA |= _BV(ADEN); // włącz ADC
}

#define ENABLE_ADC_INTERRUPT ADCSRA |= _BV(ADIE);
#define DISABLE_ADC_INTERRUPT ADCSRA &= ~_BV(ADIE);
#define START_ADC_CONVERSION ADCSRA |= _BV(ADSC);

SemaphoreHandle_t read_adc_mutex = NULL;
StaticSemaphore_t read_adc_mutex_buffer;

SemaphoreHandle_t adc_ready_semaphore = NULL;
StaticSemaphore_t adc_ready_semaphore_buffer;

ISR(ADC_vect) {
    xSemaphoreGiveFromISR(adc_ready_semaphore, NULL);
    DISABLE_ADC_INTERRUPT;
}

static uint16_t read_adc(uint8_t mux) {
    mux &= 0x0F;

    xSemaphoreTake(read_adc_mutex, portMAX_DELAY);

    ADMUX |= mux;
    ENABLE_ADC_INTERRUPT;
    START_ADC_CONVERSION;

    // Semafor jest na wejściu wzięty, więc tutaj poczekamy,
    // aż zostanie oddany przez ADC interrupt handler.
    xSemaphoreTake(adc_ready_semaphore, portMAX_DELAY);

    uint16_t adc_result = ADC;
    ADMUX &= ~mux;

    xSemaphoreGive(read_adc_mutex);

    return adc_result;
}

#define DEFINE_ADC_READ_TASK(name, mux, delay, format)           \
    static void name##_task(void* parameters) {                  \
        (void)parameters;                                        \
        while (1) {                                              \
            TickType_t previous_wake_time = xTaskGetTickCount(); \
            vTaskDelayUntil(&previous_wake_time, delay);         \
            uint16_t adc_result = read_adc(mux);                 \
            printf(format "\r\n", adc_result);                   \
        }                                                        \
    }

#define POTENTIOMETER_DELAY 642

#define POTENTIOMETER_TASK_STACK_SIZE configMINIMAL_STACK_SIZE + 200
#define POTENTIOMETER_TASK_PRIORITY 1

DEFINE_ADC_READ_TASK(potentiometer, POTENTIOMETER_MUX, POTENTIOMETER_DELAY,
    "|          %4.u |               |               |")

#define THERMISTOR_DELAY 913

#define THERMISTOR_TASK_STACK_SIZE configMINIMAL_STACK_SIZE + 200
#define THERMISTOR_TASK_PRIORITY 1

DEFINE_ADC_READ_TASK(thermistor, THERMISTOR_MUX, THERMISTOR_DELAY,
    "|               |          %4.u |               |")

#define PHOTORESISTOR_DELAY 1421

#define PHOTORESISTOR_TASK_STACK_SIZE configMINIMAL_STACK_SIZE + 200
#define PHOTORESISTOR_TASK_PRIORITY 1

DEFINE_ADC_READ_TASK(photoresistor, PHOTORESISTOR_MUX, PHOTORESISTOR_DELAY,
    "|               |               |          %4.u |")

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

void vApplicationIdleHook(void) {
    sleep_mode();
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

    sei();

    initialize_adc();

    set_sleep_mode(SLEEP_MODE_IDLE);

    read_adc_mutex = xSemaphoreCreateMutexStatic(&read_adc_mutex_buffer);
    assert(read_adc_mutex != NULL);

    adc_ready_semaphore = xSemaphoreCreateBinaryStatic(&adc_ready_semaphore_buffer);
    assert(adc_ready_semaphore != NULL);

    CREATE_STATIC_TASK(potentiometer_task, potenti,
        POTENTIOMETER_TASK_STACK_SIZE, NULL, POTENTIOMETER_TASK_PRIORITY);

    CREATE_STATIC_TASK(thermistor_task, thermis,
        THERMISTOR_TASK_STACK_SIZE, NULL, THERMISTOR_TASK_PRIORITY);

    CREATE_STATIC_TASK(photoresistor_task, photore,
        PHOTORESISTOR_TASK_STACK_SIZE, NULL, PHOTORESISTOR_TASK_PRIORITY);

    CREATE_STATIC_TASK(blinking_led_task, blnkled,
        BLINKING_LED_TASK_STACK_SIZE, NULL, BLINKING_LED_TASK_PRIORITY);

    vTaskStartScheduler();
    return 0;
}
