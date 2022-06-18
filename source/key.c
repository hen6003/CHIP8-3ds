#include <3ds.h>

bool is_pressed(char key)
{
	u32 keys_down = hidKeysHeld();

	switch (key)
	{
	case 0x1:
		return keys_down & KEY_LEFT;
	case 0x2:
		return keys_down & KEY_UP;
	case 0x3:
		return keys_down & KEY_RIGHT;
	case 0x4:
		return keys_down & KEY_Y;
	case 0x5:
		return keys_down & KEY_X;
	case 0x6:
		return keys_down & KEY_A;
	case 0x7:
		return keys_down & KEY_L && keys_down & KEY_LEFT;
	case 0x8:
		return keys_down & KEY_L && keys_down & KEY_UP;
	case 0x9:
		return keys_down & KEY_L && keys_down & KEY_RIGHT;
	case 0xA:
		return keys_down & KEY_L && keys_down & KEY_Y;
	case 0x0:
		return keys_down & KEY_L && keys_down & KEY_X;
	case 0xB:
		return keys_down & KEY_L && keys_down & KEY_A;
	case 0xC:
		return keys_down & KEY_DOWN;
	case 0xD:
		return keys_down & KEY_B;
	case 0xE:
		return keys_down & KEY_L && keys_down & KEY_DOWN;
	case 0xF:
		return keys_down & KEY_L && keys_down & KEY_B;
	}

	return false;
}

char get_pressed()
{
	u32 keys_down = hidKeysHeld();

	if (!(keys_down & KEY_L))
	{
		if (keys_down & KEY_LEFT)
			return 0x1;
		if (keys_down & KEY_UP)
			return 0x2;
		if (keys_down & KEY_RIGHT)
			return 0x3;
		if (keys_down & KEY_Y)
			return 0x4;
		if (keys_down & KEY_X)
			return 0x5;
		if (keys_down & KEY_A)
			return 0x6;
		if (keys_down & KEY_DOWN)
			return 0xC;
		if (keys_down & KEY_B)
			return 0xD;
	}
	else
	{
		if (keys_down & KEY_LEFT)
			return 0x7;
		if (keys_down & KEY_UP)
			return 0x8;
		if (keys_down & KEY_RIGHT)
			return 0x9;
		if (keys_down & KEY_Y)
			return 0xA;
		if (keys_down & KEY_X)
			return 0x0;
		if (keys_down & KEY_A)
			return 0xB;
		if (keys_down & KEY_DOWN)
			return 0xE;
		if (keys_down & KEY_B)
			return 0xF;
	}

	return -1;
}
