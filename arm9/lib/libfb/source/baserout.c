#include <nds.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "libcommon.h"

#define VARSIZE 260

#define SPACE 32
#define TAB_STOP 256
#define TRANSPARENT_VALUE 257
#define UNDERLINE 258
#define VERTBIAS 259

#define FAKE_NEWLINE 27

#define LINES_TO_RENDER 15

static uint16 **currentFont = NULL;
static uint16 *expandedFont[VARSIZE];
static uint16 defaultUnderline[] = { 8 };

static int lx = 0;
static int ly = 0;
static int inverse = 0;
static int border = 0;
static bool cVis = false;
static bool bold = false;
static bool italics = false;
static bool underline = false;
static bool strikethrough = false;
static uint16 cColor = 0;
static int cTop = -1;
static int cBottom = 1;
static int cLeft = 0;
static int italicJump = 0;
static bool twoPass = false;
static uint16 fontColor;
static uint16 highlightColor;
static int curPos = 0;
static int highlightStart = -1;
static int highlightEnd = -1;

bool libFB_bgDisabled = false;
bool libFB_bgPreserve = false;

// these cache values are for figuring out where the cursor was last time,
// so as to return the value of where the cursor would be if it were moved
// up or down
static uint16 cAbove = 0;
static uint16 cBelow = 0;

static uint16 cOldX = 0;
static uint16 cOldY = 0;

//=======================================//
//										 //
// common functions to reduce size       //
//									     //
//=======================================//

static int (*callbackFunction)(int,u32,int,int,int) = NULL;
static bool callbackEnabled = false;

// internal prototypes
inline uint32 getSpriteWidthInternal(uint16* sprite);
inline uint32 getSpriteHeightInternal(uint16* sprite);

// defined as inline instead of macro to reduce macro bugs
inline int MAXFB(int a, int b)
{
	return a > b ? a : b;
}

inline uint32 bitsFromPal(uint32 palHigh)
{
	if(palHigh < 3)
		return 1;
	if(palHigh < 5)
		return 2;
	if(palHigh < 9)
		return 3;
	if(palHigh < 17)
		return 4;
	if(palHigh < 33)
		return 5;
	if(palHigh < 65)
		return 6;
	if(palHigh < 129)
		return 7;
	return 8;
}

inline uint32 bitMaskCalc(uint32 palHigh)
{
	if(palHigh < 3)
		return 1; //0000 0001
	if(palHigh < 5)
		return 3; //0000 0011
	if(palHigh < 9)
		return 7; //0000 0111
	if(palHigh < 17)
		return 15; //0000 1111
	if(palHigh < 33)
		return 31; //0001 1111
	if(palHigh < 65)
		return 63; //0011 1111
	if(palHigh < 129)
		return 127; //0111 1111
	return 255; //1111 1111
}

static inline void dispCustomSpriteNormal(int x, int y, uint16* sprite, uint32 tc, uint32 colormask, uint16 *bgFlag, int max_X, int max_Y)
{
	//italics
	if(italics)
		x+=2;
	
	uint16* bTemp = bgFlag + (y * SCREEN_WIDTH) + x; 
	
	//get sprite width/height
	
	int width = *sprite++;
	++width;
	int height = *sprite++;
	++height;
	
	//exit if we are out of bounds
	
	if((x + width) <= 0)
		return;
	if((y + height) <= 0)
		return;
	if(x > (int)max_X)
		return;
	if(y > (int)max_Y)
		return;
	
	tc |= BIT(15); // make sure that colors match regardless of transparency bit
	
	int italicsCounter = 0;
	int i;
	int screenWidthMinusWidth = (SCREEN_WIDTH - width); // so it doesn't calc in the loop
	
	if((y + height) > max_Y) // set height accordingly so we don't have to test in the loop
		height = max_Y - y;
	
	if(!italics)
		italicJump = -1;
	
	//oops cant direct copy, gotta do transparent!
	
	for(i=0;i<height;++i) 
	{
		if((y + i) >= 0)
		{
			int j;
			for(j=0;j<width;++j) 
			{			
				if((x + j) < 0)
				{
					++bTemp;
					++sprite;
					
					continue;				
				}
				
				if((x + j) > max_X)
				{
					bTemp += width-j;
					sprite += width-j;
					
					break;
				}
				
				//just display the sprite if it isn't the damn transparency color
				if((*sprite | BIT(15)) != tc)
				{
					if(inverse)
						*bTemp = (~(*sprite) & colormask) | BIT(15);
					else
						*bTemp = ((*sprite) & colormask) | BIT(15);
				}
				
				++sprite;
				++bTemp;
			}
		}
		else
		{
			bTemp += width;
			sprite += width;
		}
		
		if(italicsCounter == italicJump)
		{
			italicsCounter = 0;
			--x;
			--bTemp;
		}
		else
		{
			++italicsCounter;
		}
		
		bTemp+= screenWidthMinusWidth;
	}	
}

static inline void dispCustomSpriteCompressed(int x, int y, uint16* sprite, uint32 tc, uint32 colormask, uint16 *bgFlag, int max_X, int max_Y)
{
	//italics
	if(italics)
		x+=2;
	
	uint16* bTemp = bgFlag + (y * SCREEN_WIDTH) + x; 
	
	++sprite; //add one to skip compression set bit
	
	//get sprite width/height
	
	int width = *sprite++;
	++width;
	int height = *sprite++;
	++height;
	
	//exit if we are out of bounds
	
	if((x + width) <= 0)
		return;
	if((y + height) <= 0)
		return;
	if(x > (int)max_X)
		return;
	if(y > (int)max_Y)
		return;
		
	//construct pallate
	
	tc |= BIT(15);
	colormask |= BIT(15);
	
	int tPallate = -1;
	uint16 pal[256];
	uint16 palHigh = *sprite; //number of entries in the pallate;
	
	int k;
	for(k=0;k<palHigh;++k)
	{
		pal[k] = (*++sprite) | BIT(15);
		if(pal[k] == tc)
			tPallate = k;
		
		if(inverse)
			pal[k] = (~pal[k] & colormask) | BIT(15);
		else
			pal[k] = pal[k] & (colormask | BIT(15));
	}
	
	uint8* pTemp = (uint8 *)++sprite;
	
	if((y + height) > max_Y) // set height accordingly so we don't have to test in the loop
		height = max_Y - y;
	
	if(!italics)
		italicJump = -1;
	
	//oops cant direct copy, gotta do transparent!
	
	uint16 bfp = bitsFromPal(palHigh);
	uint16 bmc = bitMaskCalc(palHigh);
	
	int screenWidthMinusWidth = (SCREEN_WIDTH - width);
	int italicsCounter = 0;
	int i;
	for(i=0;i<height;++i) 
	{
		if((y + i) >= 0)
		{
			int IxWidth = (i * width);
			int j;
			
			for(j=0;j<width;++j) 
			{			
				if((x + j) < 0)
				{
					++bTemp;
					
					continue;				
				}
				
				if((x + j) > (int)max_X)
				{
					bTemp += width-j;
					
					break;
				}
				
				//handle pallate
				
				int cTemp = 0;
				if(bfp == 8) 
				{
					// this is easy, each byte represents a pallate index
					cTemp = *(pTemp + (IxWidth + j));
				}
				else 
				{
					int bitPlace = (IxWidth + j) * bfp;
					
					u32 bpShiftCache = bitPlace >> 3;
					u32 bpAndCache = bitPlace & 7;
					
					cTemp = *(pTemp + bpShiftCache);
					
					if((bpAndCache + bfp) > 8) 
					{
						//we have overlapped the byte, gotta shift 8 <-- and add another byte
						cTemp = (cTemp << 8) | *(pTemp + bpShiftCache + 1);
						
						//ok now shift so the byte is on the right, 16 - (bitsfrompal + mod) bits
						
						cTemp = cTemp >> (16 - (bpAndCache + bfp));
					}
					else 
					{
						//we haven't overlapped, shift the bits --> that way 8 - (bitsfrompal + mod) places
						cTemp = cTemp >> (8 - (bpAndCache + bfp));
					}
					
					cTemp &= bmc;  //clear upper bits
				}
				
				if(cTemp != tPallate)
					*bTemp = pal[cTemp];
				
				++bTemp;
			}
		}
		else
		{
			bTemp += width;
		}
		
		if(italicsCounter == italicJump)
		{
			italicsCounter = 0;
			--x;
			--bTemp;
		}
		else
		{
			++italicsCounter;
		}
		
		bTemp+= screenWidthMinusWidth;
	}	
}

void dispCustomSprite(int x, int y, uint16* sprite, uint32 tc, uint32 colormask, uint16 *bgFlag, int max_X, int max_Y)
{
	if(!bgFlag)
		return;
	if(!sprite)
		return;
	
	// check compressed
	// splitting into two functions so theres less if statements in the loops
	
	if(*sprite == 0xFFFF)
		dispCustomSpriteCompressed(x, y, sprite, tc, colormask, bgFlag, max_X, max_Y);
	else
		dispCustomSpriteNormal(x, y, sprite, tc, colormask, bgFlag, max_X, max_Y);
} 

void xorSwap(int *x, int *y)
{
	if (x != y) 
	{
		*x ^= *y;
		*y ^= *x;
		*x ^= *y;
	}
}

void drawRect(int x, int y, int bx, int by, uint16 c, uint16 *buf, int max_y)
{
	if(!buf)
		return;
	
	if(by < 0)
		return;
	
	if(y > max_y)
		return;
	
	if(y < 0)
		y = 0;
	if(by > max_y)
		by = max_y;
	
	if(x > bx)
		xorSwap(&x, &bx);
	if(y > by)
		xorSwap(&y, &by);
	
	uint16* bTemp = buf + (y * SCREEN_WIDTH) + x;
	int width = (bx-x);
	int a;
	int b;
	c |= BIT(15);

	for(a=y;a<=by;++a) 
	{
		for(b=0;b<=width;++b) 
		{
			*bTemp = c;
			++bTemp;
		}
		bTemp += SCREEN_WIDTH - (width + 1);
	}
}

inline bool isArbitraryGlyphAvailable(u32 c, uint16 **FP)
{
	if(!FP)
		return false;
	
	if(FP[c])
		return true;
	
	return false;
}

inline bool isGlyphAvailable(u32 c)
{
	return isArbitraryGlyphAvailable(c, currentFont);
}

inline uint32 getCharWidth(u32 toDisp)
{	
	if(!isGlyphAvailable(toDisp))
		toDisp = '?';
	
	return getSpriteWidthInternal(currentFont[toDisp]);
}

inline uint32 getSpriteWidthInternal(uint16* sprite)
{
	if(*sprite == 0xFFFF)
		return (*(sprite+1)) + 1;		
	return (*sprite) + 1;
}

inline uint32 getSpriteHeightInternal(uint16* sprite)
{
	if(*sprite == 0xFFFF)
		return (*(sprite+2)) + 1;		
	return (*(sprite+1)) + 1;
}

// to stop inline warnings when linking against this lib (warnings are bad)
uint32 getSpriteWidth(uint16 *sprite)
{
	return getSpriteWidthInternal(sprite);
}

uint32 getSpriteHeight(uint16 *sprite)
{
	return getSpriteHeightInternal(sprite);
}

inline uint32 getCharHeight(u32 chr)
{
	if(!isGlyphAvailable(chr))
		return getSpriteHeightInternal(currentFont[SPACE]);
	
	return getSpriteHeightInternal(currentFont[chr]);
}

inline uint32 getTabStop()
{
	return *(currentFont[TAB_STOP]);
}

inline uint32 getUnderline()
{
	return *(currentFont[UNDERLINE]);
}

inline uint16 getTransparentColorFont()
{
	return *(currentFont[TRANSPARENT_VALUE]);
}

void setFont(uint16 **FP)
{
	if(!FP)
		return;

	static const u8 specialControlChars[] = {
		1, 2, 3, 4, 5, 6, 14, 15, 16, 17, 18, 19, 20, 21
	};
	static const u16 extendedGlyphChars[] = {
		192, 193, 194, 195, 196, 197, 198, 199, 200, 201,
		202, 203, 204, 205, 206, 207, 208, 209, 210, 211,
		212, 213, 214, 217, 218, 219, 220, 221, 222, 223,
		224, 225, 226, 227, 228, 229, 230, 231, 232, 233,
		234, 235, 236, 237, 238, 239, 240, 241, 242, 243,
		244, 245, 246, 249, 250, 251, 252, 253, 254
	};

	memset(expandedFont, 0, sizeof(expandedFont));

	for (u32 chr = 32; chr <= 126; ++chr)
	{
		expandedFont[chr] = FP[chr - 32];
	}

	expandedFont[TAB_STOP] = FP[95];
	expandedFont[TRANSPARENT_VALUE] = FP[96];
	expandedFont[UNDERLINE] = defaultUnderline;
	expandedFont[VERTBIAS] = FP[111];

	for (u32 i = 0; i < sizeof(specialControlChars); ++i)
	{
		expandedFont[specialControlChars[i]] = FP[97 + i];
		expandedFont[127 + i] = FP[97 + i];
	}

	for (u32 i = 0; i < (sizeof(extendedGlyphChars) / sizeof(extendedGlyphChars[0])); ++i)
	{
		expandedFont[extendedGlyphChars[i]] = FP[112 + i];
	}

	currentFont = expandedFont;
}

void setColor(uint16 color)
{
	fontColor = color | BIT(15);
}

void setHighlightColor(uint16 color)
{
	highlightColor = color | BIT(15);
}

void setHighlight(int start, int end)
{
	if(start > end)
	{
		highlightStart = end;
		highlightEnd = start;
	}
	else
	{
		highlightStart = start;
		highlightEnd = end;
	}
}

void clearHighlight()
{
	highlightStart = -1;
	highlightEnd = -1;
}

void setTwoPasEnabled(bool t)
{
	twoPass = t;
}

void drawInverse()
{
	inverse = 1;
}

void drawNormal()
{
	inverse = 0;
}

void setWrapToBorder()
{
	border = 1;
}

void setWrapNormal()
{
	border = 0;
}

void setBold(bool b)
{
	bold = b;
}

void setItalics(bool i)
{
	italics = i;
}

void setUnderline(bool u)
{
	underline = u;
}

void setStrikeThrough(bool s)
{
	strikethrough = s;
}

void setCallBack(int (*callback)(int,u32,int,int,int))
{
	callbackFunction = callback;
	callbackEnabled = true;
}

void clearCallBack()
{
	callbackEnabled = false;
	callbackFunction = NULL;
}

static inline int getVerticalBias(u32 c)
{
	(void)c;
	return 0;
}

uint32 getFontHeight()
{
	return getSpriteHeightInternal(currentFont[SPACE]) + 2;
}

int getStringWidth(const void *inStr, uint16 **FP)
{	
	if(!FP)
		return 0;
	if(!inStr)
		return 0;
	
	char *toDisp = (char *)inStr; // unicode changes
	int xpos = 0;
	
	// calculate it!!
	
	while(*toDisp)
	{
		switch(*toDisp)
		{
			case 9: // tab doesn't make sense in this function				
				break;
			case '\a':
				++xpos;
				break;
			case '\b':				
				--xpos;
				break;
			case '\f':
				break;
			case '\v':	
				break;
			case 13: // carriage return doesn't make sense in this function
			case 10: // lets assume they want this to be the end of the calculation
			case FAKE_NEWLINE:
				return xpos;				
				break;
			case 0:
				return xpos;
				break;
			default:
				if(!isGlyphAvailable(*toDisp))
					xpos += getSpriteWidthInternal(FP[(int)'?']);
				else				
					xpos += getSpriteWidthInternal(FP[(int)*toDisp]);
		}
		
		toDisp++;
	}
	
	return xpos;
}

uint8 getWrapPoints(int x, int y, const void *inStr, int c_x, int c_y, int c_bx, int c_by, int **pts, uint16 **FP)
{	
	char *toDisp = (char *)inStr; // unicode
	int xpos;
	u32 chr;
	int z = 0;
	int lastSpace = 0;
	bool foundSpace = false;
	
	int ptcnt = 1;
	int curPt = 0;
	
	if((*pts) != NULL)
		free((*pts));
	
	if(!FP)
		return 0;
	
	(*pts) = (int *)malloc(sizeof(int));
	
	(*pts)[0] = 0;
	
	xpos = x + c_x;
	
	// calculate it!!
	
	while(*toDisp)
	{
		chr = *toDisp;
		++lastSpace;
		++curPt;
		
		if(chr > 32)
		{
			goto charJumpGWP;
		}
		
		switch( chr )
		{	
			case 9: //tab
				z = 0;
				while(z <= xpos) 
				{
					z += *FP[TAB_STOP]; // tabstop
				}
				
				xpos = z;
				lastSpace = 0;
				break;
			case '\a':
				++xpos;
				break;
			case '\b':				
				--xpos;
				break;
			case '\f':
				break;
			case '\v':
				break;
			case 13: // carraige return/line feed
			case 10: // carriage return/line feed
			case FAKE_NEWLINE: // carriage return/line feed hack
				if(border)
					xpos = c_x;
				else
					xpos = x + c_x;
				
				foundSpace = false;
				
				(*pts) = (int *)realloc((*pts), (sizeof(int)) * (ptcnt+1));
				(*pts)[ptcnt] = curPt;
				++ptcnt;
				
				if(*toDisp == FAKE_NEWLINE)
					*toDisp = 32;
				
				break;
			case ' ':
				lastSpace = 0;
				
				foundSpace = true;
			default:
				charJumpGWP:
				if(!isArbitraryGlyphAvailable(chr, FP))
					chr = '?';
				
				xpos += getSpriteWidthInternal(FP[(int)chr]);
		}
		++toDisp;
		
		if(xpos > c_bx) 
		{
			if(foundSpace)
			{
				toDisp -= lastSpace + 1;
				curPt -= lastSpace + 1;
				*toDisp = FAKE_NEWLINE; // fake carriage return for wordwrapping
			}
			else
			{
				if(border)
					xpos = c_x + getSpriteWidthInternal(FP[(int)chr]);
				else
					xpos = x + c_x + getSpriteWidthInternal(FP[(int)chr]);
				
				(*pts) = (int *)realloc((*pts), (sizeof(int)) * (ptcnt+1));
				(*pts)[ptcnt] = curPt-1;
				++ptcnt;
			}
		}
	}
	
	return ptcnt;
}

void dispChar(int x, int y, u32 chr, uint16 *buffer, int max_X, int max_Y)
{
	int width;
	int height;
	int row;
	int col;
	uint16 *sprite;
	uint16 transparent;
	uint32 colormask;

	if(!isGlyphAvailable(chr))
		chr = '?';

	if(chr == ' ')
		return;

	sprite = currentFont[chr];
	if(!sprite)
		return;

	transparent = getTransparentColorFont() | BIT(15);
	colormask = (chr < ' ') ? 0xFFFF : fontColor;

	if(*sprite == 0xFFFF)
	{
		if(chr < ' ')
			dispCustomSprite(x, y + getVerticalBias(chr), sprite, getTransparentColorFont(), 0xFFFF, buffer, max_X, max_Y);
		else
			dispCustomSprite(x, y + getVerticalBias(chr), sprite, getTransparentColorFont(), fontColor, buffer, max_X, max_Y);
		return;
	}

	width = sprite[0] + 1;
	height = sprite[1] + 1;
	sprite += 2;
	y += getVerticalBias(chr);

	for(row = 0; row < height; ++row)
	{
		int dstY = y + row;
		if(dstY < 0 || dstY > max_Y)
			continue;

		for(col = 0; col < width; ++col)
		{
			int dstX = x + col;
			uint16 pixel;

			if(dstX < 0 || dstX > max_X)
				continue;

			pixel = sprite[(row * width) + col];
			if((pixel | BIT(15)) == transparent)
				continue;

			if(inverse)
				buffer[(dstY * SCREEN_WIDTH) + dstX] = ((~pixel) & colormask) | BIT(15);
			else if(colormask == 0xFFFF)
				buffer[(dstY * SCREEN_WIDTH) + dstX] = pixel | BIT(15);
			else
				buffer[(dstY * SCREEN_WIDTH) + dstX] = colormask;
		}
	}
}

static int calc_c_x;
static int calc_c_bx;
static int calc_x;
static int calc_border;
static int calc_tGetHeight;
static u32 calc_chr;

void calculateNewXY(int *xpos, int *ypos)
{
	if((*xpos + getCharWidth(calc_chr)) > calc_c_bx) // wordwrap didn't work on this, line too long, lets fake it!
	{
		if(calc_border)
			*xpos = calc_c_x;
		else
			*xpos = calc_x + calc_c_x;
		
		*ypos += (calc_tGetHeight + 2);
	}
}

int dispString(int x, int y, const void *inStr, uint16 *buffer, uint32 enableClipping, int c_x, int c_y, int c_bx, int c_by)
{
	char *toDisp = (char *)inStr; // unicode change
	char *Disp = toDisp;
	char *lastDisp = toDisp;
	int xpos;
	int ypos;
	u32 chr;
	int z;
	int lastSpace = 0;
	unsigned int cP = 0;
	bool aFound = false;
	bool bFound = false;
	bool foundSpace = false;
	bool linesRendered = 0;
	u32 tGetHeight = getCharHeight(SPACE);
	u32 rememberOldHeight = tGetHeight;
	
	xpos = x + c_x;
	ypos = y + c_y;
	
	int trailX = xpos;
	int trailY = ypos;
	
	// calculate it!!
	
	int curPosition = 0;	
	int tEnding = c_by;
	
	if(!enableClipping)
		tEnding = 192;
	
	if(enableClipping) 
	{		
		while(*toDisp)
		{
			chr = *toDisp;			
			
			if(twoPass)
			{
				if(callbackEnabled)
				{
					if(callbackFunction(curPosition++, chr, 1,xpos,ypos) == -1)
						return ypos - (y + c_y) + tGetHeight;
				}
			}
			
			tGetHeight = MAXFB(getCharHeight(chr), tGetHeight);
			
			++toDisp;
			++lastSpace;
			
			if(chr > 32)
			{
				goto charJumpDS1;
			}
			
			switch(chr)
			{	
				case 9: //tab
					z = 0;
					while(z <= xpos) 
					{
						z += getTabStop();
					}
					
					xpos = z;
					lastSpace = 0;
					break;
				case '\a':
					++xpos;
					break;
				case '\b':				
					--xpos;
					break;
				case '\f':
					++ypos;
					if(ypos >= tEnding)
						goto Loop1Exit;
					
					break;
				case '\v':				
					--ypos;
					break;
				case 13: // carraige return/line feed
				case 10: // carriage return/line feed
				case FAKE_NEWLINE: // carriage return/line feed hack
					if(border)
						xpos = c_x;
					else
						xpos = x + c_x;
					ypos += tGetHeight + 2;
					if(ypos >= tEnding)
					{
						linesRendered++;
						
						if(linesRendered > LINES_TO_RENDER)
						{
							goto Loop1Exit;
						}
					}
					
					tGetHeight = getSpriteHeightInternal(currentFont[SPACE]);
					
					foundSpace = false;
					
					break;
				case ' ':
					lastSpace = 0;
					
					foundSpace = true;
				default:
					charJumpDS1:
					if(!isGlyphAvailable(chr))
						chr = '?';
					
					xpos += getSpriteWidthInternal(currentFont[chr]);
			}
			
			if(xpos > c_bx) 
			{
				if(foundSpace)
				{
					toDisp -= lastSpace + 1;
					curPosition -= lastSpace + 1;
					*toDisp = FAKE_NEWLINE; // fake carriage return for wordwrapping
				}
				else
				{
					if(border)
						xpos = c_x + getSpriteWidthInternal(currentFont[chr]);
					else
						xpos = x + c_x + getSpriteWidthInternal(currentFont[chr]);
					ypos += tGetHeight + 2;
					if(ypos >= tEnding)
						goto Loop1Exit;
					
					tGetHeight = MAXFB(getSpriteHeightInternal(currentFont[SPACE]), getSpriteHeightInternal(currentFont[chr]));
				}
			}
		}
Loop1Exit:
		
		// draw it!		
		xpos = x + c_x;
		ypos = y + c_y;
		lastDisp = toDisp;
		toDisp = Disp;
		
		curPosition = 0;
	}
	
	tGetHeight = rememberOldHeight;
	
	while(*toDisp)
	{
		chr = *toDisp;
		
		if(callbackEnabled)
		{
			calc_c_x = c_x;
			calc_c_bx = c_bx;
			calc_x = x;
			calc_border = border;
			calc_tGetHeight = tGetHeight;
			calc_chr = chr;
			
			if(callbackFunction(curPosition++, chr, 2,xpos,ypos) == -1)
				return ypos - (y + c_y) + tGetHeight;
		}
		
		trailX = xpos;
		trailY = ypos;
		
		tGetHeight = MAXFB(getCharHeight(chr), tGetHeight);
		italicJump = getCharHeight(chr) / 3;
		
		if(chr > 32)
		{
			goto charJumpDS2;
		}
		
		switch( chr )
		{
			case 9: //tab
				z = 0;
				while(z <= xpos) 
				{
					z += getTabStop();
				}
				
				if(cP >= highlightStart && cP < highlightEnd)
				{
					drawRect(xpos, ypos, z - 1, ypos + getUnderline() + 1, highlightColor, buffer, c_by);
				}
				
				xpos = z;
				break;
			case '\a':		
				++xpos;
				break;
			case '\b':
				--xpos;
				break;
			case '\f':
				++ypos;
				if(ypos >= tEnding)
					goto Loop2Exit;
				break;
			case '\v':				
				--ypos;
				break;
			case 13: // carraige return/line feed
			case 10: // carriage return/line feed
			case FAKE_NEWLINE: // carriage return/line feed hack				
				if(cVis)
				{
					if(!aFound)
					{
						if(ypos + (tGetHeight + 2) == cOldY)
						{
							cAbove = cP;
							aFound = true;
						}				
					}
					
					if(!bFound)
					{
						if(ypos == (int)(cOldY + (tGetHeight + 2)))
						{
							cBelow = cP;
							bFound = true;
						}
					}
				}
				
				if(border)
					xpos = c_x;
				else
					xpos = x + c_x;
				ypos += (tGetHeight + 2);				
				
				if(ypos >= tEnding)
					goto Loop2Exit;
				
				tGetHeight = getSpriteHeightInternal(currentFont[SPACE]);
				
				//reset FAKE_NEWLINE back to space.
				
				if(*toDisp == FAKE_NEWLINE)
					*toDisp = 32;
				break;
			case ' ':
				if(cP >= highlightStart && cP < highlightEnd)
				{
					drawRect(xpos, ypos, xpos + getCharWidth(chr) - 1, ypos + getUnderline() + 1, highlightColor, buffer, c_by);
				}
				
				if(underline)
				{
					int tmpY = ypos + getUnderline();
					
					if(tmpY >= c_y && tmpY <= c_by)
						drawRect(xpos, tmpY, xpos + getCharWidth(chr), tmpY, fontColor, buffer, c_by);
				}
				
				if(strikethrough)
				{
					int tmpY = ypos + (getUnderline() / 2) + 1;
					
					if(tmpY >= c_y && tmpY <= c_by)
						drawRect(xpos, tmpY, xpos + getCharWidth(chr), tmpY, fontColor, buffer, c_by);
				}
				
				xpos += getCharWidth(chr);
				break;
			default:	
				charJumpDS2:
				if(!isGlyphAvailable(chr))
					chr = '?';
				
				if(enableClipping)
				{
					if((xpos + getCharWidth(chr)) > c_bx) // wordwrap didn't work on this, line too long, lets fake it!
					{	
						if(cVis)
						{
							if(ypos + (tGetHeight + 2) == cOldY && !aFound)
							{
								cAbove = cP;
								aFound = true;
							}				
							
							if(ypos == (int)(cOldY + (tGetHeight + 2)) && !bFound)
							{
								cBelow = cP;
								bFound = true;
							}
						}
						
						if(border)
							xpos = c_x;
						else
							xpos = x + c_x;
						ypos += (tGetHeight + 2);
						
						if(ypos >= tEnding)
							goto Loop2Exit;
						
						trailX = xpos;
						trailY = ypos;
						
						tGetHeight = MAXFB(getSpriteHeightInternal(currentFont[SPACE]), getSpriteHeightInternal(currentFont[chr]));
					}
				}
				
				if(cP >= highlightStart && cP < highlightEnd)
				{
					drawRect(xpos, ypos, xpos + getCharWidth(chr) - 1, ypos + getUnderline() + 1, highlightColor, buffer, c_by);
				}
				
				dispChar(xpos, ypos, chr, buffer, 255, c_by);
				
				if(bold && chr != 0xB7) // fix for bullets
					dispChar(xpos+1, ypos, chr, buffer, 255, c_by);
				
				if(underline)
				{
					int tmpY = ypos + getUnderline();
					
					if(tmpY >= c_y && tmpY <= c_by)
						drawRect(xpos, tmpY, xpos + getCharWidth(chr), tmpY, fontColor, buffer, c_by);
				}
				
				if(strikethrough)
				{
					int tmpY = ypos + (getUnderline() / 2) + 1;
					
					if(tmpY >= c_y && tmpY <= c_by)
						drawRect(xpos, tmpY, xpos + getCharWidth(chr), tmpY, fontColor, buffer, c_by);
				}
					
				xpos += getCharWidth(chr);
				break;
		}
		
		// here instead for callbacks
		lx = xpos;
		ly = ypos;
		
		++toDisp;
		
		if(cVis)
		{
			if(cP == curPos)
			{				
				//draw cursor!
				drawRect(trailX+cLeft,trailY+cTop,trailX+cLeft,trailY+tGetHeight+cBottom,cColor, buffer, c_by);
				
				cOldX = trailX;
				cOldY = trailY;
			}
			
			if(ypos + (tGetHeight + 2) == cOldY && !aFound)
			{
				// we are on the line above!
				
				if(cOldX >= trailX && cOldX < xpos) // the cursor closely matches this position!
				{
					if(cOldX > ((trailX + xpos)/2))
						cAbove = cP + 1;
					else
						cAbove = cP;
					aFound = true;
				}				
			}
			
			if(ypos == (int)(cOldY + (tGetHeight + 2)) && !bFound)
			{
				// we are on the line below
				
				if(cOldX >= trailX && cOldX < xpos) // the cursor closely matches this position!
				{
					if(cOldX > ((trailX + xpos)/2))
						cBelow = cP + 1;
					else
						cBelow = cP;
					bFound = true;
				}				
			}
		}
		
		++cP;
	}
Loop2Exit:
	
	while(*toDisp && toDisp <= lastDisp)
	{
		if(*toDisp == FAKE_NEWLINE)
			*toDisp = 32;
		
		++toDisp;
	}
	
	if(cVis)	
	{
		if(ypos == (int)(cOldY + (tGetHeight + 2)) && !bFound)
		{
			// we are on the line below and we hit the end of the line
			
			cBelow = cP;
			bFound = true;
		}
		if(cP == curPos)
		{						
			trailX = xpos;
			trailY = ypos;
			
			//draw cursor!
			drawRect(trailX+cLeft,trailY+cTop,trailX+cLeft,trailY+tGetHeight+cBottom,cColor,buffer, c_by);
			
			cOldX = trailX;
			cOldY = trailY;
		}
		
		if(!aFound) cAbove = curPos;
		if(!bFound) cBelow = curPos;
	}
	
	return ypos - (y + c_y) + tGetHeight;
}

int getLastX()
{
	return lx;
}

int getLastY()
{
	return ly;
}

void setCursorPos(uint32 pos)
{
	curPos = pos;
}

void showCursor()
{
	cVis = true;
}

void hideCursor()
{
	cVis = false;
}

void setCursorProperties(uint16 color, int top, int height, int left)
{
	cColor = color | BIT(15);
	cTop = top;
	cBottom = height;
	cLeft = left;
}

uint32 getPositionAbove()
{
	return cAbove;
}
	
uint32 getPositionBelow()
{
	return cBelow;
}

void disableBackgroundDraw()
{
	libFB_bgDisabled = true;
}

void preserveBackground()
{
	libFB_bgPreserve = true;
}
