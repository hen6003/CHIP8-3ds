#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h> 
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

void tick(struct chip_8 *state, bool *redraw)
{
	if (state->delay_timer)
		state->delay_timer--;
	if (state->sound_timer)
	{
		state->sound_timer--;
		audio_play();
	}
	
	int opcode = state->memory[state->program_counter] << 8 | state->memory[state->program_counter+1];
	state->program_counter += 2;

	int X   = ((opcode & 0x0F00) >> 8);
	int Y   = ((opcode & 0x00F0) >> 4);
	int N   = ( opcode & 0x000F);
	int NN  = ( opcode & 0x00FF);
	int NNN =   opcode & 0x0FFF;

	//if ((opcode & 0xF000) >> 12 != 1)
	printf("%#x: %04x: ", state->program_counter-2, opcode);

	switch ((opcode & 0xF000) >> 12)
	{
	case 0:
		switch (NNN)
		{
		case 0x0E0:
			clear_screen(state->display);
			printf("CLS");
			break;
		case 0x0EE:
			state->program_counter = stack_pop(state->stack);
			printf("RET %#x", state->program_counter);
			break;
		default:
			printf("Invalid machine call");
			break;
		}
		break;
	case 1: // Jump
		state->program_counter = NNN;
		printf("JMP %#x", state->program_counter);
		break;
	case 2: // Subroutine
		stack_push(state->stack, state->program_counter);

		state->program_counter = NNN;
		printf("CAL %#x", state->program_counter);
		break;
	case 3: // Skip equal
		if (state->registers[X] == NN)
			state->program_counter += 2;
		printf("SEQ r[%#x]=%#x, %#x", X, state->registers[X], NN);
		break;
	case 4: // Skip not equal
		if (state->registers[X] != NN)
			state->program_counter += 2;
		printf("SNE r[%#x]=%#x, %#x", X, state->registers[X], NN);
		break;
	case 5: // Skip registers equal
		if (state->registers[X] == state->registers[Y])
			state->program_counter += 2;
		printf("SEQ r[%#x]=%x, r[%#x]=%x", X, state->registers[X], Y, state->registers[Y]);
		break;
	case 6: // Set
		state->registers[X] = NN;
		printf("SET r[%#x], %#x", X, NN);
		break;
	case 7: // Add
		state->registers[X] += NN;
		printf("ADD r[%#x], %#x", X, NN);
		break;
	case 9: // Skip registers not equal
		if (state->registers[X] != state->registers[Y])
			state->program_counter += 2;
		printf("SNE r[%#x]=%x, r[%#x]=%x", X, state->registers[X], Y, state->registers[Y]);
		break;
	case 8:
		bool set_vf = false;
		switch (N)
		{
		case 0:
			state->registers[X] = state->registers[Y];
			printf("SET r[%#x], r[%#x]", X, Y);
			break;
		case 1:
			state->registers[X] |= state->registers[Y];
			printf("OR  r[%#x], r[%#x]", X, Y);
			break;
		case 2:
			state->registers[X] &= state->registers[Y];
			printf("AND r[%#x], r[%#x]", X, Y);
			break;
		case 3:
			state->registers[X] ^= state->registers[Y];
			printf("XOR r[%#x], r[%#x]", X, Y);
			break;
		case 4:
			set_vf = ((uint16_t) state->registers[X] + (uint16_t) state->registers[Y]) > UINT8_MAX;
			state->registers[X] += state->registers[Y];
			printf("ADD r[%#x], r[%#x]", X, Y);
			break;
		case 5:
			set_vf = state->registers[X] >= state->registers[Y];
			state->registers[X] -= state->registers[Y];
			printf("SUB r[%#x], r[%#x]", X, Y);
			break;
		case 6:
			set_vf = state->registers[X] & 1;
			state->registers[X] >>= 1;
			printf("SHR r[%#x]", X);
			break;
		case 7:
			set_vf = state->registers[Y] >= state->registers[X];
			state->registers[X] = state->registers[Y] - state->registers[X];
			printf("SUB r[%#x], r[%#x]", Y, X);
			break;
		case 0xE:
			set_vf = state->registers[X] & 0x80;
			state->registers[X] <<= 1;
			printf("SHL r[%#x]", X);
			break;
		default:
			printf("Unknown opcode");
			break;
		}
		state->registers[0xF] = set_vf;
		break;
	case 0xA: // Set index
		state->index = NNN;
		printf("SET I, %#x", NNN);
		break;
	case 0xB: // Jump with offset
		state->program_counter = NNN + state->registers[0];
		printf("JMP %#x + r[0x0]", NNN);
		break;
	case 0xC: // Random
		state->registers[X] = rand() & NN;
		printf("RND r[%#x] %#x", X, NN);
	case 0xD: // Draw
		*redraw = true;
		int x = state->registers[X] % 64;
		int y = state->registers[Y] % 32;

		state->registers[0xF] = false;

		printf("DRW %x x %x (%d), %#x", x, y, N, state->index);

		for (int row_i = 0; row_i < N; row_i++)
		{
			char row = state->memory[state->index + row_i];

			for (int xline = 0; xline < 8; xline++)
			{
				if ((row & (0x80 >> xline)) != 0)
				{
					if (state->display[x][y])
						state->registers[0xF] = true;

					state->display[x][y] ^= 1;
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
		char key = state->registers[X];
		bool pressed = is_pressed(key);
		printf("Pressed: %#x: %d", key, pressed);
		switch (NN)
		{
		case 0x9E:
			if (pressed)
				state->program_counter += 2;
			break;
		case 0xA1:
			if (!pressed)
				state->program_counter += 2;
			break;
		}
		break;
	case 0xF:
		switch (NN)
		{
		case 0x07:
			state->registers[X] = state->delay_timer;
			break;
		case 0x15:
			state->delay_timer = state->registers[X];
			break;
		case 0x18:
			state->sound_timer = state->registers[X];
			break;
		case 0x1E:
			state->index += state->registers[X];

			if (state->index > 0xFFF) // Overflow
				state->registers[0xF] = true;
			break;
		case 0x0A: // Get key
			char key = get_pressed();
			if (key != 255)
				state->registers[X] = key;
			else
				state->program_counter -= 2;

			break;
		case 0x29: // Font
			char character = state->registers[X] & 0xF;

			state->index = CHIP_8_FONT_ADDRESS + character * 5;
			break;
		case 0x33:
			char number = state->registers[X];

			state->memory[state->index+0] = number / 100;
			state->memory[state->index+1] = number / 10 % 10;
			state->memory[state->index+2] = number % 10;
			break;
		case 0x55:
			for (int i = 0; i <= X; i++)
				state->memory[state->index + i] = state->registers[i];
			break;
		case 0x65:
			for (int i = 0; i <= X; i++)
				state->registers[i] = state->memory[state->index + i];
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

struct file_entry
{
	char name[256];
	char path[1024];
};

void file_list_add_dir(struct file_entry *list[], int *found, char *dir_name)
{
	DIR *d = opendir(dir_name);
	struct dirent *dir;

	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
			if (dir->d_type == DT_REG)
			{
				(*found)++;
				*list = realloc(*list, (*found + 1) * sizeof(struct file_entry));
				
				strcpy((*list)[*found-1].name, dir->d_name);
				snprintf((*list)[*found-1].path, 1024, "%s%s", dir_name, dir->d_name);
			}
		}

		closedir(d);
	}

}

struct file_entry *file_list()
{
	struct file_entry *list = NULL;

	int found = 0;
	
	file_list_add_dir(&list, &found, "romfs:/");
	file_list_add_dir(&list, &found, "/chip8/");

	// Mark end of list
	*list[found].name = 0;

	return list;
}

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

	// Init romfs
	Result rc = romfsInit();

	if (rc)
		printf("Failed to load romfs!!!\n");

	// Top screen graphics
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);

	atexit(C2D_Fini);
	atexit(C3D_Fini);
	
	C2D_Prepare();

	C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

	// Initialize console on top screen. Using NULL as the second argument tells the console library to use the internal console structure as current one
	consoleInit(GFX_BOTTOM, &bottom_console);

	/// Main loop
	bool pause;
	bool redraw = true;
	bool running = false;
	int hovered_file = 0;

	struct chip_8 state;

	struct file_entry *list = file_list();
	int list_size = 0;
	while (list[list_size++].name[0] != 0) ;
	list_size--;

	while (aptMainLoop())
	{
		// Scan all the inputs. This should be done once for each frame
		hidScanInput();
		
		u32 keys_down = hidKeysDown();

		if (running)
		{
			if (keys_down & KEY_START)
			{
				pause = !pause;
				redraw = true;
			}

			if (!pause)
				tick(&state, &redraw);

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

			if (keys_down & KEY_SELECT)
			{
				running = false;
				redraw = true;
				printf("\e[2J");
			}
		}
		else // ROM select
		{
			if (redraw)
			{
				printf("\e[1;1HRom select:\n");
					
				for (int i = 0; i < list_size; i++)
				{
					printf(" ");

					if (i == hovered_file)
						printf("\e[44m");

					printf("%s\e[0m\n", list[i].name);
				}

				redraw = false;
			}

			if (keys_down & KEY_A)
			{
				redraw = true;
				running = true;
				pause = false;

				init_chip_8(&state, list[hovered_file].path);
			}

			if (keys_down & KEY_DOWN && hovered_file < list_size - 1)
			{
				hovered_file++;
				redraw = true;
			}
				
			if (keys_down & KEY_UP && hovered_file > 0)
			{
				hovered_file--;
				redraw = true;
			}
		}
	}
}
