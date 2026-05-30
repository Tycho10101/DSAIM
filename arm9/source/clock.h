#ifndef _CLOCK_
#define _CLOCK_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nds.h"

class rClock
{
public:
    rClock();
    ~rClock();
    void rSetFont(unsigned short int **fnt) { font = fnt; update = true; }
    void rSetPosition(int x, int y) { drawX = x; drawY = y; update = true; }
    void rSetColor(unsigned short c) { color = c; update = true; }
    
    int rDraw();
    int rNeedsDrawn();
private:
    char min[2];
    unsigned short color;
    unsigned short int **font;
    int drawX;
    int drawY;
    bool update;
    char clockString[25];
};

#endif


