#include <nds.h>
#include <string.h>
#include <stdlib.h>
#include "bg_compat.h"
#include "libcommon.h"

static int buffers;
static int bgEnabled;

static uint16 *bgFrameBuffer = NULL;
static uint16 *framebuffer1 = NULL;
static uint16 *framebuffer2 = NULL;

static int clip_x;
static int clip_y;
static int clip_bx;
static int clip_by;
static int fb_orientation;

static int clip;
static int update;
static uint16 bgColor;

static bool fbEnabled = false;

extern bool libFB_bgDisabled;
extern bool libFB_bgPreserve;

#define FB_EXIT_IF_DISABLED() if(!fbEnabled) { return; }

//=======================================//
//										 //
// frame buffer and other such libraries //
//										 //
//=======================================//

void fb_init()
{
	videoSetMode(MODE_5_2D | DISPLAY_BG2_ACTIVE | DISPLAY_BG3_ACTIVE);
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
	vramSetBankB(VRAM_B_MAIN_BG_0x06020000);
	
	//set the control registers and rot scale information on the extended rotation backgrounds
    BG2_CR = BG_BMP16_256x256 | BG_BMP_BASE(0) | BG_PRIORITY(2);
    BG2_XDX = 1 << 8;
    BG2_XDY = 0;
    BG2_YDY = 1 << 8;
    BG2_YDX = 0;
    BG2_CX = 0;
    BG2_CY = 0;
   
    BG3_CR = BG_BMP16_256x256 | BG_BMP_BASE(8) | BG_PRIORITY(3);
    BG3_XDX = 1 << 8;
    BG3_XDY = 0;
    BG3_YDY = 1 << 8;
    BG3_YDX = 0;
    BG3_CX = 0;
    BG3_CY = 0;
	
	fb_orientation = ORIENTATION_0;
	
	buffers = 0;
	bgEnabled = 0;
	bgColor = 0;
	update = 0;
	framebuffer1 = (uint16 *)BG_BMP_RAM(0);
	framebuffer2 = (uint16 *)BG_BMP_RAM(8);
	memset(framebuffer1, 0, BUFFER_SIZE * 2);
	memset(framebuffer2, 0, BUFFER_SIZE * 2);
	
	if(bgFrameBuffer)
		free(bgFrameBuffer);
	bgFrameBuffer = NULL;
	
	fbEnabled = true;
	
	fb_enableClipping();
	fb_setDefaultClipping();
}

void fb_enableClipping()
{
	clip = 1;
}

void fb_disableClipping()
{
	clip = 0;
}

void fb_clear()
{
	FB_EXIT_IF_DISABLED();
	
	memset(framebuffer2, 0, BUFFER_SIZE * 2);
	
	update = 1;
}

uint16 *fb_backBuffer()
{
	return framebuffer2;
}

void fb_setDefaultClipping()
{
	clip_x = 0;
	clip_y = 0;
	clip_bx = MAX_X;
	clip_by = MAX_Y;
}

void fb_setOrientation(int orientation)
{
	FB_EXIT_IF_DISABLED();
	
	if(orientation == fb_orientation)
	{
		return;
	}
	
	switch(orientation)
	{
		case ORIENTATION_0:	
			BG2_XDX = 1 << 8;
			BG2_XDY = 0;
			BG2_YDX = 0;
			BG2_YDY = 1 << 8;
			BG2_CX = 0;
			BG2_CY = 0;
		   
			BG3_XDX = 1 << 8;
			BG3_XDY = 0;
			BG3_YDX = 0;
			BG3_YDY = 1 << 8;
			BG3_CX = 0;
			BG3_CY = 0;
			break;
		case ORIENTATION_90:
			BG2_XDX = 0;
			BG2_XDY = -1 << 8;
			BG2_YDX = 1 << 8;
			BG2_YDY = 0;
			BG2_CX = 191 << 8;
			BG2_CY = 0 << 8;
		   
			BG3_XDX = 0;
			BG3_XDY = -1 << 8;
			BG3_YDX = 1 << 8;
			BG3_YDY = 0;
			BG3_CX = 191 << 8;
			BG3_CY = 0 << 8;
			break;
		case ORIENTATION_180:
			BG2_XDX = -1 << 8;
			BG2_XDY = 0;
			BG2_YDX = 0;
			BG2_YDY = -1 << 8;
			BG2_CX = 255 << 8;
			BG2_CY = 191 << 8;
		   
			BG3_XDX = -1 << 8;
			BG3_XDY = 0;
			BG3_YDX = 0;
			BG3_YDY = -1 << 8;
			BG3_CX = 255 << 8;
			BG3_CY = 191 << 8;
			break;
		case ORIENTATION_270:
			BG2_XDX = 0;
			BG2_XDY = 1 << 8;
			BG2_YDX = -1 << 8;
			BG2_YDY = 0;
			BG2_CX = 0 << 8;
			BG2_CY = 255 << 8;
		   
			BG3_XDX = 0;
			BG3_XDY = 1 << 8;
			BG3_YDX = -1 << 8;
			BG3_YDY = 0;
			BG3_CX = 0 << 8;
			BG3_CY = 255 << 8;
			break;
	}
	
	fb_orientation = orientation;
}

void fb_swapBuffers()
{
	FB_EXIT_IF_DISABLED();
	
	int i;
	u32 *tSrc;
	u32 *tDst;
	
	if(!buffers) 
	{
		framebuffer1 = (uint16 *)BG_BMP_RAM(8);
		framebuffer2 = (uint16 *)BG_BMP_RAM(0);
		
		BG2_CR = BG_BMP16_256x256 | BG_BMP_BASE(0) | BG_PRIORITY(3);
		BG3_CR = BG_BMP16_256x256 | BG_BMP_BASE(8) | BG_PRIORITY(2);
		
		buffers = 1;
	}
	else 
	{
		framebuffer1 = (uint16 *)BG_BMP_RAM(0);
		framebuffer2 = (uint16 *)BG_BMP_RAM(8);
		
		BG2_CR = BG_BMP16_256x256 | BG_BMP_BASE(0) | BG_PRIORITY(2);
		BG3_CR = BG_BMP16_256x256 | BG_BMP_BASE(8) | BG_PRIORITY(3);
		
		buffers = 0;
	}
	
	if(libFB_bgPreserve)
	{
		tSrc = (u32 *)framebuffer1;
		tDst = (u32 *)framebuffer2;
		
		for(i=0;i<BUFFER_SIZE>>1;++i)
			*tDst++ = *tSrc++;
	}
	
	else if(!libFB_bgDisabled)
	{
		if(!bgEnabled) 
		{
			u32 tColor = bgColor | (bgColor << 16);
			tDst = (u32 *)framebuffer2;
			
			for(i=0;i<BUFFER_SIZE>>1;++i)
				*tDst++ = tColor;
		}
		else 
		{
			tSrc = (u32 *)bgFrameBuffer;
			tDst = (u32 *)framebuffer2;
			
			for(i=0;i<BUFFER_SIZE>>1;++i)
				*tDst++ = *tSrc++;
		}		
	}
	
	libFB_bgPreserve = false;
	libFB_bgDisabled = false;
	
	fb_setDefaultClipping();
}

void fb_setBG(uint16 *buffer)
{
	FB_EXIT_IF_DISABLED();
	
	bgEnabled = 1;
	
	bgFrameBuffer = (uint16 *)malloc(MAX_X * MAX_Y * 2);
	memcpy(bgFrameBuffer, buffer, MAX_X * MAX_Y * 2);
	memcpy(framebuffer2, bgFrameBuffer, MAX_X * MAX_Y * 2);
	
	update = 1;
}

void fb_setBGColor(uint16 color)
{
	FB_EXIT_IF_DISABLED();
	
	int i;
	
	bgColor = color | BIT(15);
	
	for(i=0;i<MAX_X*MAX_Y;i++)
		framebuffer2[i] = bgColor;
	
	update = 1;
}

void fb_eraseBG()
{
	FB_EXIT_IF_DISABLED();
	
	bgEnabled = 0;
	fb_setBGColor(bgColor);
	
	if(bgFrameBuffer)
		free(bgFrameBuffer);
	bgFrameBuffer = NULL;
	
	update = 1;
}	

void fb_setClipping(int x, int y, int bx, int by)
{
	clip_x = x;
	clip_y = y;
	clip_bx = bx;
	clip_by = by;
}

void fb_setPixel(int x, int y, uint16 color)
{	
	FB_EXIT_IF_DISABLED();
	
	if(x >= MAX_X || y >= MAX_Y)
		return;
	
	framebuffer2[(y * SCREEN_WIDTH) + x] = color | BIT(15);
	
	update = 1;
}

//first 2 bytes of sprite are width,height
//sprite length is width*height*2+4
//sprite is bitmap style

void fb_dispCustomSprite(int x, int y, uint16* sprite, uint16 tc, uint16 colormask)
{
	FB_EXIT_IF_DISABLED();
	
	dispCustomSprite(x, y, sprite, tc, colormask, framebuffer2, MAX_X, MAX_Y);
	
	update = 1;
} 

void fb_dispSprite(int x, int y, uint16* sprite, uint16 tc)
{
	fb_dispCustomSprite(x, y, sprite, tc, 0xFFFF);
	
	update = 1;
}

void fb_dispChar(int x, int y, int toDisp)
{	
	FB_EXIT_IF_DISABLED();
	
	update = 1;
	
	dispChar(x, y, toDisp, framebuffer2, MAX_X, MAX_Y);
}

//this function expects a null terminated string

void fb_dispString(int x, int y, const void *toDisp)
{
	FB_EXIT_IF_DISABLED();
	
	dispString(x, y, toDisp, framebuffer2, clip, clip_x, clip_y, clip_bx, clip_by);
	
	update = 1;
}

void fb_drawRect(int x, int y, int bx, int by, uint16 c)
{
	FB_EXIT_IF_DISABLED();
	
	drawRect(x, y, bx, by, c, framebuffer2, MAX_Y);	
	
	update = 1;
}

void fb_drawBox(int x, int y, int bx, int by, uint16 c)
{
	FB_EXIT_IF_DISABLED();
	
	fb_drawRect(x, y, bx, y, c);
	fb_drawRect(x, by, bx, by, c);
	fb_drawRect(x, y, x, by, c);
	fb_drawRect(bx, y, bx, by, c);	
}

bool fb_needsUpdate()
{
	if(update == 1)
		return true;
	
	return false;
}

void fb_setUpdate()
{
	update = 1;
}

bool fb_isbgEnabled()
{
	return (bgEnabled == 1);
}

uint16 *fb_returnFrameBuffer()
{
	return bgFrameBuffer;
}

void fb_drawFilledRect(int x, int y, int bx, int by, uint16 borderColor, uint16 fillColor)
{
	FB_EXIT_IF_DISABLED();
	
	fb_drawBox(x,y,bx,by,borderColor);
	fb_drawRect(x+1,y+1,bx-1,by-1,fillColor);
}
