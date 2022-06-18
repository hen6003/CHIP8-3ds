#include <string.h>
#include <stdio.h>

#include "chip-8.h"

const char CHIP_8_FONT[] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void init_chip_8(struct chip_8 *state, char file_name[])
{
	/// Init memory
	memset(state->memory, 0, 0xFF);
	memcpy(&state->memory[CHIP_8_FONT_ADDRESS], CHIP_8_FONT, sizeof CHIP_8_FONT); // Load font

	/// Read file
	FILE *file = fopen(file_name, "rb");

	// Get file size
	fseek(file,0,SEEK_END);
	off_t size = ftell(file);
	fseek(file,0,SEEK_SET);

	// Read file
	fread(&state->memory[CHIP_8_PROGRAM_ADDRESS], 1, size, file);

	//close the file because we like being nice and tidy
	fclose(file);

	/// Init display
	clear_screen(state->display);

	/// Init stack
	init_stack(state->stack);

	/// Init registers
	memset(state->registers, 0, 0xF);
	state->program_counter = CHIP_8_PROGRAM_ADDRESS;
	state->index = 0;
	state->delay_timer = 0;
	state->sound_timer = 0;
}

void clear_screen(bool display[64][32])
{
	for (int x = 0; x < 64; x++)
		for (int y = 0; y < 32; y++)
			display[x][y] = false;
}
