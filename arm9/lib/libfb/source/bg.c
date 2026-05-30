#include <nds.h>
#include <nds/arm9/background.h>
#include <string.h>
#include "bg_compat.h"
#include "libcommon.h"

static uint16 bgBuffer[BUFFER_SIZE];
static uint16 bg_buffer[BUFFER_SIZE];

static int bg_clip_x;
static int bg_clip_y;
static int bg_clip_bx;
static int bg_clip_by;
static int bg_orientation;

static int bg_clip;
static int bg_update;

static uint16 bg_bgColor;
static int bg_bgEnabled;

extern bool libFB_bgDisabled;
extern bool libFB_bgPreserve;

//=======================================//
//										 //
// bgbuffer and other such libraries     //
//										 //
//=======================================//

void bg_init()
{
	videoSetModeSub(MODE_5_2D | DISPLAY_BG2_ACTIVE);
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);
	
	SUB_BG2_CR = BG_BMP16_256x256 | BG_BMP_BASE(0);
	SUB_BG2_XDX = 1 << 8;
	SUB_BG2_XDY = 0;
	SUB_BG2_YDX = 0;
	SUB_BG2_YDY = 1 << 8;
	SUB_BG2_CY = 0;
	SUB_BG2_CX = 0;
	
	bg_update = 0;
	bg_orientation = ORIENTATION_0;
	
	bg_bgColor = (1 << 15);
	bg_bgEnabled = 0;
	memset(bg_buffer, bg_bgColor, BUFFER_SIZE * 2);
	memset(bgBuffer, bg_bgColor, BUFFER_SIZE * 2);
	bg_enableClipping();
	bg_setDefaultClipping();
}

void bg_enableClipping()
{
	bg_clip = 1;
}

void bg_disableClipping()
{
	bg_clip = 0;
}

void bg_clear()
{
	memset(bg_buffer, 0, BUFFER_SIZE * 2);
	
	bg_update = 1;
}

uint16 *bg_backBuffer()
{
	return bg_buffer;
}

void bg_setDefaultClipping()
{
	bg_clip_x = 0;
	bg_clip_y = 0;
	bg_clip_bx = MAX_X;
	bg_clip_by = MAX_Y;
}

void bg_setOrientation(int orientation)
{
	if(orientation == bg_orientation)
	{
		return;
	}
	
	switch(orientation)
	{
		case ORIENTATION_0:		   
			SUB_BG2_XDX = 1 << 8;
			SUB_BG2_XDY = 0;
			SUB_BG2_YDX = 0;
			SUB_BG2_YDY = 1 << 8;
			SUB_BG2_CX = 0;
			SUB_BG2_CY = 0;
			break;
		case ORIENTATION_90:
			SUB_BG2_XDX = 0;
			SUB_BG2_XDY = -1 << 8;
			SUB_BG2_YDX = 1 << 8;
			SUB_BG2_YDY = 0;
			SUB_BG2_CX = 191 << 8;
			SUB_BG2_CY = 0;
			break;
		case ORIENTATION_180:
			SUB_BG2_XDX = -1 << 8;
			SUB_BG2_XDY = 0;
			SUB_BG2_YDX = 0;
			SUB_BG2_YDY = -1 << 8;
			SUB_BG2_CX = 255 << 8;
			SUB_BG2_CY = 191 << 8;
			break;
		case ORIENTATION_270:
			SUB_BG2_XDX = 0;
			SUB_BG2_XDY = 1 << 8;
			SUB_BG2_YDX = -1 << 8;
			SUB_BG2_YDY = 0;
			SUB_BG2_CX = 0;
			SUB_BG2_CY = 255 << 8;
			break;
	}
	
	bg_orientation = orientation;
}

void bg_swapBuffers()
{	
	int i;
	u32 *tSrc = (u32 *)bg_buffer;
	u32 *tDst = (u32 *)BG_BMP_RAM_SUB(0);
	
	for(i = 0; i < (BUFFER_SIZE / 2); ++i)
	{
		tDst[i] = tSrc[i];
	}
	
	if(!libFB_bgDisabled && !libFB_bgPreserve)
	{
		tSrc = (u32 *)bgBuffer;
		tDst = (u32 *)bg_buffer;
		
		for(i = 0; i < (BUFFER_SIZE / 2); ++i)
		{
			tDst[i] = tSrc[i];
		}
	}
	
	libFB_bgDisabled = false;
	libFB_bgPreserve = false;
	
	bg_setDefaultClipping();
}

void bg_setBG(uint16 *buffer)
{
	bg_bgEnabled = 1;
	memcpy(bgBuffer, buffer, MAX_X * MAX_Y * 2);
	memcpy(bg_buffer, bgBuffer, MAX_X * MAX_Y * 2);
	
	bg_update = 1;
}

void bg_setBGColor(uint16 color)
{
	int i;
	
	bg_bgColor = color | BIT(15);
	
	for(i=0;i<MAX_X*MAX_Y;i++)
		bgBuffer[i] = bg_bgColor;
	memcpy(bg_buffer, bgBuffer, MAX_X * MAX_Y * 2);
}

void bg_eraseBG()
{
	bg_bgEnabled = 0;
	bg_setBGColor(bg_bgColor);
	
	bg_update = 1;
}	

void bg_setClipping(int x, int y, int bx, int by)
{
	bg_clip_x = x;
	bg_clip_y = y;
	bg_clip_bx = bx;
	bg_clip_by = by;
}

void bg_setPixel(int x, int y, uint16 color)
{	
	if(x >= MAX_X || y >= MAX_Y)
		return;
	
	bg_buffer[(y * SCREEN_WIDTH) + x] = color | BIT(15);
	
	bg_update = 1;
}

//first 2 bytes of sprite are width,height
//sprite length is width*height*2+4
//sprite is bitmap style

void bg_dispCustomSprite(int x, int y, uint16* sprite, uint16 tc, uint16 colormask)
{	
	dispCustomSprite(x, y, sprite, tc, colormask, bg_buffer, MAX_X, MAX_Y);
	
	bg_update = 1;
} 

void bg_dispSprite(int x, int y, uint16* sprite, uint16 tc)
{
	bg_dispCustomSprite(x, y, sprite, tc, 0xFFFF);
	
	bg_update = 1;
}
	

void bg_dispChar(int x, int y, int toDisp)
{	
	bg_update = 1;
	
	dispChar(x, y, toDisp, bg_buffer, MAX_X, MAX_Y);	
}

//this function expects a null terminated string

void bg_dispString(int x, int y, const void *toDisp)
{
	dispString(x, y, toDisp, bg_buffer, bg_clip, bg_clip_x, bg_clip_y, bg_clip_bx, bg_clip_by);
	
	bg_update = 1;
}

void bg_drawRect(int x, int y, int bx, int by, uint16 c)
{
	drawRect(x, y, bx, by, c, bg_buffer, MAX_Y);
	
	bg_update = 1;
}

void bg_drawBox(int x, int y, int bx, int by, uint16 c)
{
	bg_drawRect(x, y, bx, y, c);
	bg_drawRect(x, by, bx, by, c);
	bg_drawRect(x, y, x, by, c);
	bg_drawRect(bx, y, bx, by, c);	
}

bool bg_needsUpdate()
{
	if(bg_update)
		return true;
	
	return false;
}

void bg_setUpdate()
{
	bg_update = 1;
}

bool bg_isbgEnabled()
{
	return (bg_bgEnabled == 1);
}

uint16 *bg_returnFrameBuffer()
{
	return bgBuffer;
}

void bg_drawFilledRect(int x, int y, int bx, int by, uint16 borderColor, uint16 fillColor)
{
	bg_drawBox(x,y,bx,by,borderColor);
	bg_drawRect(x+1,y+1,bx-1,by-1,fillColor);
}
