#include <stdint.h>

#define STACK_SIZE 8

void init_stack(uint16_t stack[STACK_SIZE]);
void stack_push(uint16_t stack[STACK_SIZE], uint16_t data);
uint16_t stack_pop(uint16_t stack[STACK_SIZE]);
