#include <nds.h>
#include <stdlib.h>
#include "libcommon.h"

/* Poll-based VBlank wait — calico threadIrqWait broken with ds9-legacy.specs */
static inline void waitForVBlank(void)
{
    while ((*(volatile unsigned short*)0x04000006) >= 192) {}
    while ((*(volatile unsigned short*)0x04000006) < 192) {}
}
#define swiWaitForVBlank() waitForVBlank()

static uint16 B_TL[] = { 0xFFFF, 5, 5, 6, 31, 22197, 24311, 27482, 29596, 32767, // width, height, # of pallate entries, pallate entries
0x0200, 0x2440, 0x49A0, 0x124C, 0x259C, 0x0A39, 0x5072 };

static uint16 B_T[] = { 0xFFFF, 0, 5, 6, 22197, 24311, 27482, 29596, 32767, 0, // width, height, # of pallate entries, pallate entries
0x3905, 0x0040 };

static uint16 B_TR[] = { 0xFFFF, 5, 5, 6, 22197, 31, 24311, 27482, 29596, 32767, // width, height, # of pallate entries, pallate entries
0x9200, 0x0050, 0x8096, 0x3406, 0x9101, 0x2CA0, 0x008D };

static uint16 B_R[] = { 0xFFFF, 5, 0, 6, 0, 32767, 29596, 27482, 24311, 22197, // width, height, # of pallate entries, pallate entries
0x3905, 0x0040 };

static uint16 B_BR[] = { 0xFFFF, 5, 5, 6, 32767, 29596, 27482, 24311, 22197, 31, // width, height, # of pallate entries, pallate entries
0xA704, 0x4E09, 0x9C42, 0x3995, 0x7225, 0x644B, 0xD0B6 };

static uint16 B_B[] = { 0xFFFF, 0, 5, 6, 0, 32767, 29596, 27482, 24311, 22197, // width, height, # of pallate entries, pallate entries
0x3905, 0x0040 };

static uint16 B_BL[] = { 0xFFFF, 5, 5, 6, 22197, 24311, 27482, 29596, 32767, 31, // width, height, # of pallate entries, pallate entries
0x3705, 0x2900, 0x01BA, 0x804E, 0xB40A, 0x6D00, 0x00B4 };

static uint16 B_L[] = { 0xFFFF, 5, 0, 6, 22197, 24311, 27482, 29596, 32767, 0, // width, height, # of pallate entries, pallate entries
0x3905, 0x0040 };

static uint16 TRANS_VALUE[] = { 31 };
static uint16 BACKGROUND_VALUE[] = { 0 };

static uint16* BoxPointers[] = { 	B_TL, B_T, B_TR, B_R, B_BR, B_B, B_BL, B_L, TRANS_VALUE, BACKGROUND_VALUE };

// generic defines
extern void bg_setUpdate();
extern void fb_setUpdate();
extern bool fb_isbgEnabled();
extern bool bg_isbgEnabled();
extern uint16 *fb_returnFrameBuffer();
extern uint16 *bg_returnFrameBuffer();

uint16 getTransparentColorBox()
{
	return *(BoxPointers[8]);
}

uint16 getBGColorBox()
{
	return *(BoxPointers[9]);
}

// defines for FB screen

void fb_setDialogBox(int x, int y, int width, int height)
{		
	uint16 i;
	
	fb_setClipping(x + getSpriteWidth(BoxPointers[0]), y + getSpriteHeight(BoxPointers[0]), (x + width) - (getSpriteWidth(BoxPointers[0]) * 2), (y + height) - (getSpriteHeight(BoxPointers[0]) * 2));
	
	fb_dispCustomSprite(x,y,BoxPointers[0],getTransparentColorBox(),0xFFFF);
	fb_dispCustomSprite((x + width - getSpriteWidth(BoxPointers[0])),y,BoxPointers[2],getTransparentColorBox(),0xFFFF);
	fb_dispCustomSprite((x + width - getSpriteWidth(BoxPointers[0])),(y + height - getSpriteHeight(BoxPointers[0])),BoxPointers[4],getTransparentColorBox(),0xFFFF);
	fb_dispCustomSprite(x,(y + height - getSpriteHeight(BoxPointers[0])),BoxPointers[6],getTransparentColorBox(),0xFFFF);
	
	for(i=0;i < (width-(getSpriteWidth(BoxPointers[0]) * 2));i++)
	{
		fb_dispCustomSprite(x + getSpriteWidth(BoxPointers[0])+i,y,BoxPointers[1],getTransparentColorBox(),0xFFFF);
		fb_dispCustomSprite(x + getSpriteWidth(BoxPointers[0])+i,(y + height - getSpriteHeight(BoxPointers[0])),BoxPointers[5],getTransparentColorBox(),0xFFFF);
	}
	
	for(i=0;i < (height-(getSpriteHeight(BoxPointers[0]) * 2));i++)
	{
		fb_dispCustomSprite(x,y+getSpriteHeight(BoxPointers[0])+i,BoxPointers[7],getTransparentColorBox(),0xFFFF);
		fb_dispCustomSprite((x + width - getSpriteWidth(BoxPointers[0])),y+getSpriteHeight(BoxPointers[0])+i,BoxPointers[3],getTransparentColorBox(),0xFFFF);		
	}
	
	fb_drawRect(x + getSpriteWidth(BoxPointers[0]), y+getSpriteHeight(BoxPointers[0]), x + (width-(getSpriteWidth(BoxPointers[0]))), y + (height-(getSpriteHeight(BoxPointers[0]))), 0);
	
	fb_setUpdate();
}

void fb_dispAnimatedBox(int x, int y, int width, int height, int tx, int ty, const void *strDisp)
{
	uint16 w;
	uint16 a;
	uint16 b;
	
	uint16 *bgFrameBufferTemp = (uint16 *)malloc(BUFFER_SIZE * 2);
	
	w = 25;
	
	if(fb_isbgEnabled()) {
		dmaCopy(fb_returnFrameBuffer(), bgFrameBufferTemp, BUFFER_SIZE * 2);		
		dmaCopy(fb_backBuffer(), fb_returnFrameBuffer(), BUFFER_SIZE * 2);
	}
	
	while( w < width || w < height ) {
		if( w > width ) {
			a = width;
		}
		else {
			a = w;
		}
		
		if( w > height ) {
			b = height;
		}
		else {
			b = w;
		}
		
		fb_setDialogBox(x, y, a, b);
		swiWaitForVBlank();
		fb_swapBuffers();
		
		w+=15;
	}
	
	fb_setDialogBox(x, y, width, height);	
	fb_dispString(tx, ty, strDisp);
	
	if(fb_isbgEnabled()) {				
		dmaCopy(bgFrameBufferTemp, fb_returnFrameBuffer(), BUFFER_SIZE * 2);
	}
	
	free(bgFrameBufferTemp);
	
	fb_setUpdate();
}

// defines for BG screen

void bg_setDialogBox(int x, int y, int width, int height)
{	
	uint16 i;
	
	bg_setClipping(x + getSpriteWidth(BoxPointers[0]), y + getSpriteHeight(BoxPointers[0]), (x + width) - (getSpriteWidth(BoxPointers[0]) * 2), (y + height) - (getSpriteHeight(BoxPointers[0]) * 2));
	
	bg_dispCustomSprite(x,y,BoxPointers[0],getTransparentColorBox(),0xFFFF);
	bg_dispCustomSprite((x + width - getSpriteWidth(BoxPointers[0])),y,BoxPointers[2],getTransparentColorBox(),0xFFFF);
	bg_dispCustomSprite((x + width - getSpriteWidth(BoxPointers[0])),(y + height - getSpriteHeight(BoxPointers[0])),BoxPointers[4],getTransparentColorBox(),0xFFFF);
	bg_dispCustomSprite(x,(y + height - getSpriteHeight(BoxPointers[0])),BoxPointers[6],getTransparentColorBox(),0xFFFF);
	
	for(i=0;i < (width-(getSpriteWidth(BoxPointers[0]) * 2));i++)
	{
		bg_dispCustomSprite(x + getSpriteWidth(BoxPointers[0])+i,y,BoxPointers[1],getTransparentColorBox(),0xFFFF);
		bg_dispCustomSprite(x + getSpriteWidth(BoxPointers[0])+i,(y + height - getSpriteHeight(BoxPointers[0])),BoxPointers[5],getTransparentColorBox(),0xFFFF);
	}
	
	for(i=0;i < (height-(getSpriteHeight(BoxPointers[0]) * 2));i++)
	{
		bg_dispCustomSprite(x,y+getSpriteHeight(BoxPointers[0])+i,BoxPointers[7],getTransparentColorBox(),0xFFFF);
		bg_dispCustomSprite((x + width - getSpriteWidth(BoxPointers[0])),y+getSpriteHeight(BoxPointers[0])+i,BoxPointers[3],getTransparentColorBox(),0xFFFF);		
	}
	
	bg_drawRect(x + getSpriteWidth(BoxPointers[0]), y+getSpriteHeight(BoxPointers[0]), x + (width-(getSpriteWidth(BoxPointers[0]))), y + (height-(getSpriteHeight(BoxPointers[0]))), 0);
	
	bg_setUpdate();
}

void bg_dispAnimatedBox(int x, int y, int width, int height, int tx, int ty, const void *strDisp)
{
	uint16 w;
	uint16 a;
	uint16 b;
	
	uint16 *bgBufferTemp = (uint16 *)malloc(BUFFER_SIZE * 2);
	
	w = 25;
	
	if(bg_isbgEnabled()) {
		dmaCopy(bg_returnFrameBuffer(), bgBufferTemp, BUFFER_SIZE * 2);		
		dmaCopy(bg_backBuffer(), bg_returnFrameBuffer(), BUFFER_SIZE * 2);
	}
	
	while( w < width || w < height ) {
		if( w > width ) {
			a = width;
		}
		else {
			a = w;
		}
		
		if( w > height ) {
			b = height;
		}
		else {
			b = w;
		}
		
		bg_setDialogBox(x, y, a, b);
		swiWaitForVBlank();
		bg_swapBuffers();
		
		w+=15;
	}
	
	bg_setDialogBox(x, y, width, height);	
	bg_dispString(tx, ty, strDisp);
	
	if(bg_isbgEnabled()) {				
		dmaCopy(bgBufferTemp, bg_returnFrameBuffer(), BUFFER_SIZE * 2);
	}
	
	free(bgBufferTemp);
	
	bg_setUpdate();
}
