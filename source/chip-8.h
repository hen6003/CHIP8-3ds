#include <stdbool.h>
#include <stdint.h>

#include "stack.h"

#define CHIP_8_PROGRAM_ADDRESS 0x200
#define CHIP_8_FONT_ADDRESS 0x050

struct chip_8
{
	char memory[0xFFF];
	bool display[64][32];
	char registers[0x10];
	uint16_t stack[STACK_SIZE];
	uint16_t program_counter;
	uint16_t index;
	char delay_timer;
	char sound_timer;
};

extern const char CHIP_8_FONT[];

void init_chip_8(struct chip_8 *state, char program[], uint16_t program_size);
void clear_screen(bool display[64][32]);
