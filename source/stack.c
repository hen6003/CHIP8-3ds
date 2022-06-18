#include <string.h>

#include "stack.h"

void init_stack(uint16_t stack[STACK_SIZE])
{
	memset(stack, 0xFF, STACK_SIZE * sizeof(uint16_t));
}

void stack_push(uint16_t stack[STACK_SIZE], uint16_t data)
{	
	for (uint16_t i = 0; i < STACK_SIZE; i++)
		if (stack[i] == 0xFFFF)
		{
			stack[i] = data;
			break;
		}
}

uint16_t stack_pop(uint16_t stack[STACK_SIZE])
{	
	for (uint16_t i = STACK_SIZE - 1; i >= 0; i--)
		if (stack[i] != 0xFFFF)
		{
			uint16_t ret = stack[i];
			stack[i] = 0xFFFF;
			return ret;
		}

	return 0xFFFF;
}
