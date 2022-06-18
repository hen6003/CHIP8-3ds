#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <3ds.h>
#include <citro2d.h>

#include "chip-8.h"
#include "text.h"
#include "key.h"
#include "audio.h"

#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240

#define PIXEL_SIZE (SCREEN_WIDTH / 64.0)
#define Y_OFFSET (SCREEN_HEIGHT / PIXEL_SIZE / 2.0)

void draw_display(bool display[64][32], u32 clr_on, u32 clr_off)
{	
	for (int x = 0; x < 64; x++)
		for (int y = 0; y < 32; y++)
			C2D_DrawRectSolid(x * PIXEL_SIZE, y * PIXEL_SIZE + Y_OFFSET, 0, PIXEL_SIZE, PIXEL_SIZE,
					  display[x][y] ? clr_on : clr_off);
}

char PROGRAM[] =
{
	0x12, 0x9f, 0xfc, 0xfc, 0x80, 0xa2, 0x02, 0xdd, 0xc1, 0x00, 0xee, 0xa2, 0x04, 0xdb, 0xa1, 0x00,
	0xee, 0xa2, 0x03, 0x60, 0x02, 0x61, 0x05, 0x87, 0x00, 0x86, 0x10, 0xd6, 0x71, 0x71, 0x08, 0x6f,
	0x38, 0x8f, 0x17, 0x4f, 0x00, 0x12, 0x17, 0x70, 0x02, 0x6f, 0x10, 0x8f, 0x07, 0x4f, 0x00, 0x12,
	0x15, 0x00, 0xee, 0x22, 0x05, 0x7d, 0x04, 0x22, 0x05, 0x00, 0xee, 0x22, 0x05, 0x7d, 0xfc, 0x22,
	0x05, 0x00, 0xee, 0x80, 0x80, 0x40, 0x01, 0x68, 0xff, 0x40, 0xff, 0x68, 0x01, 0x5a, 0xc0, 0x22,
	0x53, 0x00, 0xee, 0x80, 0xb0, 0x70, 0xfb, 0x61, 0xf8, 0x80, 0x12, 0x70, 0x05, 0xa2, 0x03, 0xd0,
	0xa1, 0x00, 0xee, 0x22, 0x0b, 0x8b, 0x94, 0x8a, 0x84, 0x22, 0x0b, 0x4b, 0x00, 0x69, 0x01, 0x4b,
	0x3f, 0x69, 0xff, 0x4a, 0x00, 0x68, 0x01, 0x4a, 0x1f, 0x68, 0xff, 0x4f, 0x01, 0x22, 0x43, 0x4a,
	0x1f, 0x22, 0x85, 0x00, 0xee, 0x00, 0xe0, 0x6b, 0x1e, 0x6a, 0x14, 0x22, 0x05, 0x22, 0x0b, 0x22,
	0x11, 0x00, 0xee, 0xfe, 0x07, 0x3e, 0x00, 0x12, 0x93, 0x6e, 0x04, 0xfe, 0x15, 0x00, 0xee, 0x6d,
	0x1e, 0x6c, 0x1e, 0x6b, 0x40, 0x6a, 0x1d, 0xc9, 0x01, 0x49, 0x00, 0x69, 0xff, 0x68, 0xff, 0x22,
	0x05, 0x22, 0x0b, 0x22, 0x11, 0x60, 0x07, 0xe0, 0xa1, 0x22, 0x3b, 0x60, 0x09, 0xe0, 0xa1, 0x22,
	0x33, 0x22, 0x63, 0x22, 0x93, 0x12, 0xb5
};

int main(int argc, char **argv)
{
	PrintConsole bottom_console;

	u32 clr_off   = C2D_Color32(0x11, 0x11, 0x11, 0xFF);
	u32 clr_on    = C2D_Color32(0xDD, 0xDD, 0xDD, 0xFF);
		     
	u32 clr_clear = C2D_Color32(0x00, 0x00, 0x00, 0xFF);

	/// Init random
	srand(time(NULL));
	
	/// Init screens
	gfxInitDefault();
	atexit(gfxExit);

	text_init();
	atexit(text_exit);

	// Init sound
	audio_init();
	atexit(audio_exit);
	audio_play();

	// Initialize keyboard on bottom screen
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);

	atexit(C2D_Fini);
	atexit(C3D_Fini);
	
	C2D_Prepare();

	C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

	// Initialize console on top screen. Using NULL as the second argument tells the console library to use the internal console structure as current one
	consoleInit(GFX_BOTTOM, &bottom_console);

	/// Main loop
	bool pause = false;
	bool redraw = true;

	struct chip_8 state;
	init_chip_8(&state, PROGRAM, sizeof PROGRAM);

	while (aptMainLoop())
	{
		// Scan all the inputs. This should be done once for each frame
		hidScanInput();
		
		u32 kDown = hidKeysDown();
		
		if (kDown & KEY_START)
		{
			pause = !pause;
			redraw = true;
		}

		if (!pause)
		{
			if (state.delay_timer)
				state.delay_timer--;
			if (state.sound_timer)
			{
				state.sound_timer--;
				audio_play();
			}
			
			int opcode = state.memory[state.program_counter] << 8 | state.memory[state.program_counter+1];
			state.program_counter += 2;

			int X   = ((opcode & 0x0F00) >> 8);
			int Y   = ((opcode & 0x00F0) >> 4);
			int N   = ( opcode & 0x000F);
			int NN  = ( opcode & 0x00FF);
			int NNN =   opcode & 0x0FFF;

			//if ((opcode & 0xF000) >> 12 != 1)
			printf("%#x: %04x: ", state.program_counter-2, opcode);

			switch ((opcode & 0xF000) >> 12)
			{
			case 0:
				switch (NNN)
				{
				case 0x0E0:
					clear_screen(state.display);
					printf("CLS");
					break;
				case 0x0EE:
					state.program_counter = stack_pop(state.stack);
					printf("RET %#x", state.program_counter);
					break;
				default:
					printf("Invalid machine call");
					break;
				}
				break;
			case 1: // Jump
				state.program_counter = NNN;
				printf("JMP %#x", state.program_counter);
				break;
			case 2: // Subroutine
				stack_push(state.stack, state.program_counter);

				state.program_counter = NNN;
				printf("CAL %#x", state.program_counter);
				break;
			case 3: // Skip equal
				if (state.registers[X] == NN)
					state.program_counter += 2;
				printf("SEQ r[%#x]=%#x, %#x", X, state.registers[X], NN);
				break;
			case 4: // Skip not equal
				if (state.registers[X] != NN)
					state.program_counter += 2;
				printf("SNE r[%#x]=%#x, %#x", X, state.registers[X], NN);
				break;
			case 5: // Skip registers equal
				if (state.registers[X] == state.registers[Y])
					state.program_counter += 2;
				printf("SEQ r[%#x]=%x, r[%#x]=%x", X, state.registers[X], Y, state.registers[Y]);
				break;
			case 6: // Set
				state.registers[X] = NN;
				printf("SET r[%#x], %#x", X, NN);
				break;
			case 7: // Add
				state.registers[X] += NN;
				printf("ADD r[%#x], %#x", X, NN);
				break;
			case 9: // Skip registers not equal
				if (state.registers[X] != state.registers[Y])
					state.program_counter += 2;
				printf("SNE r[%#x]=%x, r[%#x]=%x", X, state.registers[X], Y, state.registers[Y]);
				break;
			case 8:
				switch (N)
				{
				case 0:
					state.registers[X] = state.registers[Y];
					printf("SET r[%#x], r[%#x]", X, Y);
					break;
				case 1:
					state.registers[X] |= state.registers[Y];
					printf("OR  r[%#x], r[%#x]", X, Y);
					break;
				case 2:
					state.registers[X] &= state.registers[Y];
					printf("AND r[%#x], r[%#x]", X, Y);
					break;
				case 3:
					state.registers[X] ^= state.registers[Y];
					printf("XOR r[%#x], r[%#x]", X, Y);
					break;
				case 4:
					state.registers[0xF] = ((uint16_t) state.registers[X] + (uint16_t) state.registers[Y]) > UINT8_MAX;
					state.registers[X] += state.registers[Y];
					printf("ADD r[%#x], r[%#x]", X, Y);
					break;
				case 5:
					state.registers[0xF] = state.registers[X] > state.registers[Y];
					state.registers[X] -= state.registers[Y];
					printf("SUB r[%#x], r[%#x]", X, Y);
					break;
				case 6:
					state.registers[0xF] = state.registers[X] & 1;
					state.registers[X] >>= 1;
					printf("SHR r[%#x]", X);
					break;
				case 7:
					state.registers[0xF] = state.registers[Y] > state.registers[X];
					state.registers[Y] -= state.registers[X];
					printf("SUB r[%#x], r[%#x]", Y, X);
					break;
				case 0xE:
					state.registers[0xF] = state.registers[X] & 0x80;
					state.registers[X] <<= 1;
					printf("SHL r[%#x]", X);
					break;
				default:
					printf("Unknown opcode");
					break;
				}
				break;
			case 0xA: // Set index
				state.index = NNN;
				printf("SET I, %#x", NNN);
				break;
			case 0xB: // Jump with offset
				state.program_counter = NNN + state.registers[0];
				printf("JMP %#x + r[0x0]", NNN);
				break;
			case 0xC: // Random
				state.registers[X] = rand() & NN;
				printf("RND r[%#x] %#x", X, NN);
			case 0xD: // Draw
				redraw = true;
				int x = state.registers[X] % 64;
				int y = state.registers[Y] % 32;

				state.registers[0xF] = false;

				printf("DRW %x x %x (%d), %#x", x, y, N, state.index);

				for (int row_i = 0; row_i < N; row_i++)
				{
					char row = state.memory[state.index + row_i];

					for (int xline = 0; xline < 8; xline++)
					{
						if ((row & (0x80 >> xline)) != 0)
						{
							if (state.display[x][y])
								state.registers[0xF] = true;

							state.display[x][y] ^= 1;
						}
						x++;
						if (x >= 64) break;
					}
					y++;
					x-=8;
					if (y >= 32) break;
				}
				break;
			case 0xE:
				char key = state.registers[X];
				bool pressed = is_pressed(key);
				printf("Pressed: %#x: %d", key, pressed);
				switch (NN)
				{
				case 0x9E:
					if (pressed)
						state.program_counter += 2;
					break;
				case 0xA1:
					if (!pressed)
						state.program_counter += 2;
					break;
				}
				break;
			case 0xF:
				switch (NN)
				{
				case 0x07:
					state.registers[X] = state.delay_timer;
					break;
				case 0x15:
					state.delay_timer = state.registers[X];
					break;
				case 0x18:
					state.sound_timer = state.registers[X];
					break;
				case 0x1E:
					state.index += state.registers[X];

					if (state.index > 0xFFF) // Overflow
						state.registers[0xF] = true;
					break;
				case 0x0A: // Get key
					char key = get_pressed();
					if (key != -1)
						state.registers[X] = key;
					else
						state.program_counter -= 2;

					break;
				case 0x29: // Font
					char character = state.registers[X] & 0xF;

					state.index = CHIP_8_FONT_ADDRESS + character;
					break;
				case 0x33:
					char number = state.registers[X];

					state.memory[state.index+0] = number / 100;
					state.memory[state.index+1] = number / 10 % 10;
					state.memory[state.index+2] = number % 10;
					break;
				case 0x55:
					for (int i = 0; i <= X; i++)
						state.memory[state.index + i] = state.registers[i];
					break;
				case 0x65:
					for (int i = 0; i <= X; i++)
						state.registers[i] = state.memory[state.index + i];
					break;
				default:
					printf("Unknown opcode");
					break;
				}
				break;
			default:
				printf("Unknown opcode");
				break;
			}
			printf("\n");
		}

		// Display game
		if (redraw)
		{
			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C2D_TargetClear(top, clr_clear);
			C2D_SceneBegin(top);

			draw_display(state.display, clr_on, clr_off);

			if (pause)
				show_popup(1, clr_on);

			C3D_FrameEnd(0);

			redraw = false;
		}
	}
}
