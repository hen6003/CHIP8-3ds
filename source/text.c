#include <citro2d.h>

C2D_TextBuf g_staticBuf;
C2D_Text g_staticText[2];

void text_init(void)
{
	// Create two text buffers: one for static text, and another one for
	// dynamic text - the latter will be cleared at each frame.
	g_staticBuf  = C2D_TextBufNew(4096); // support up to 4096 glyphs in the buffer

	// Parse the static text strings
	//C2D_TextParse(&g_staticText[0], g_staticBuf, "Game over!");
	C2D_TextParse(&g_staticText[1], g_staticBuf, "Paused");

	// Optimize the static text strings
	C2D_TextOptimize(&g_staticText[0]);
	C2D_TextOptimize(&g_staticText[1]);
}

void text_exit(void)
{
	// Delete the text buffer
	C2D_TextBufDelete(g_staticBuf);
}

void show_popup(size_t static_index, u32 clr)
{
	float size = .8f;

	C2D_DrawText(&g_staticText[static_index], C2D_WithColor,
		     0, 0, 0.5f, size, size, clr);
}
