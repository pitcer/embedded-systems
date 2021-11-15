#include <avr/io.h>
#include <inttypes.h>

uint16_t measure_empty(void) {
    uint16_t start_time = TCNT1;
    uint16_t end_time = TCNT1;
    return end_time - start_time;
}

#define DEFINE_EMPTY_ASSIGNMENT(type)                \
    uint16_t measure_empty_assignment_##type(void) { \
        uint16_t start_time = TCNT1;                 \
        volatile type _ = 1;                         \
        uint16_t end_time = TCNT1;                   \
        return end_time - start_time;                \
    }

DEFINE_EMPTY_ASSIGNMENT(int8_t);
DEFINE_EMPTY_ASSIGNMENT(int16_t);
DEFINE_EMPTY_ASSIGNMENT(int32_t);
DEFINE_EMPTY_ASSIGNMENT(int64_t);
DEFINE_EMPTY_ASSIGNMENT(float);

#define DEFINE_OPERATION_FUNCTION(name, operation, type)      \
    uint16_t measure_##name##_##type(type left, type right) { \
        uint16_t start_time = TCNT1;                          \
        volatile type _ = left operation right;               \
        uint16_t end_time = TCNT1;                            \
        return end_time - start_time;                         \
    }

#define DEFINE_OPERATIONS_FUNCTIONS(type)    \
    DEFINE_OPERATION_FUNCTION(add, +, type); \
    DEFINE_OPERATION_FUNCTION(mul, *, type); \
    DEFINE_OPERATION_FUNCTION(div, /, type);

DEFINE_OPERATIONS_FUNCTIONS(int8_t);
DEFINE_OPERATIONS_FUNCTIONS(int16_t);
DEFINE_OPERATIONS_FUNCTIONS(int32_t);
DEFINE_OPERATIONS_FUNCTIONS(int64_t);
DEFINE_OPERATIONS_FUNCTIONS(float);
