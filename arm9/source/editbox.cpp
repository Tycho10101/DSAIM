#include <string.h>
#include <stdio.h>
#include "nds.h"
#include <../lib/libfb/libcommon.h>
#include <stdarg.h>
#include "editbox.h"
#include "globalDefs.h"
#include "wificonnect.h"
#include "../lib/rTOC2/wifidebug.h"
#include "debug.h"
#ifdef WIFIDEBUG
#include "../lib/rTOC2/wifidebug.h"
#define WPRINT(format, args...)  rWifiDebug::rPrint("Keyboard %s :" format,__func__, ##args)
#else
#define WPRINT(format, args...)
#endif

rEditBox::rEditBox(char *theText, char *okText, char *cancelText, int x, int y, int len, int id, unsigned short int ** fnt):rDialog(theText, okText, cancelText, x, y, id, fnt, true)
{
    textInput = new rUserInput(1, 1000);
    textInput->rSetFont(fnt);
    textX = 5;
    textLen = len;
    if (textLen < 16)
        textLen = 50;
    currentPlace = 0;
    rCalcPos();
} 

rEditBox::~rEditBox()
{
    delete textInput;
}

void rEditBox::rCalcPos()
{
    int i, j;
    int len = 0;
    int fHeight;
    int boxHeight;
    fHeight = font[0][1]+1;
    boxHeight = ( (dialogList->getTotalLines() * (fHeight+4))+ fHeight+4 )+4+30;
    int y = drawY;
    if (y+boxHeight >= 192)
    {
        y = boxHeight-10;
    }
    y+=4;
    y+=(fHeight+4)*dialogList->getTotalLines();
    y+=6+(fHeight+4);
    finalLen = -1;
    okPosY = -1;
    okPosY = y;
    okPosX = -1;
    cancelPosX = -1;
    okLen = -1;
    cancelLen = -1;

    textX = (256-textLen)/2;
    if (!sOK && sCancel)
    {
        sOK = sCancel;
        sCancel = NULL;
        for (i=0; i<(int)strlen(sOK); i++)
            len += font[sOK[i]-32][0]+1;
        okPosX = (256-len)/2;
        okLen = len;
    }
    else if (sOK && !sCancel)
    {
        for (i=0; i<(int)strlen(sOK); i++)
            len += font[sOK[i]-32][0]+1;
        okPosX = (256-len)/2;
        okLen = len;
    }
    else if (sOK && sCancel)
    {
        for (i=0; i<(int)strlen(sOK); i++)
            len += font[sOK[i]-32][0]+1;
        okPosX = (128-len)/2;
        okLen = len;
        len=0;

        for (i=0; i<(int)strlen(sCancel); i++)
            len += font[sCancel[i]-32][0]+1;
        cancelPosX = (384-len)/2;
        cancelLen = len;
    }

    char *pch;
    for (j=0; j<dialogList->getTotalLines(); j++)
    {
        len = 0;
        pch = dialogList->getLine(j);
        for (i=0; i<(int)strlen(pch); i++)
            len += font[pch[i]-32][0]+1;
        dialogList->setColor((short int)(((256-4)-(len))/2), j); //use color to store xPosition;
    }
    if (cancelLen >= okLen)
        finalLen = cancelLen;
    else
        finalLen = okLen;
}

int rEditBox::rNeedsDrawn()
{
    if (textInput->rNeedsDrawn() != 0)
        update = true;
    if (!update)
	return 0;
    update = false;
    return 2;
}

int rEditBox::rDraw()
{
    if (font == NULL)
        return 0;
    int x = drawX;
    int y = drawY;
    update = false;
    if (x >= 100)
    {
        x = 100;
    }
    int fHeight = font[0][1]+1;
    int boxHeight = ( (dialogList->getTotalLines() * (fHeight+4) + (fHeight+4) ) )+4+30;
    int i,j;
    if (y+boxHeight >= 192)
    {
        y = boxHeight-10;
    }
    setFont(font);
    for (i=0; i<192; i++)
    {
	if ( (i % 2) == 0)
	{
	    for (j=0; j<256; j+=2)
		fb_setPixel(j,i,RGB15(0,0,0));
	}
	else
	{
	    for (j=1; j<256; j+=2)
		fb_setPixel(j,i,RGB15(0,0,0));
	}
    }
    fb_drawRect(x,y,256-x, y+boxHeight, RGB15(0,0,0));
    fb_drawRect(x+2,y+2,256-x-2, y+boxHeight-2, RGB15(31,31,31));
    
    y = y+4;
    for (i=0; i<dialogList->getTotalLines(); i++)
    {
        fb_dispString(dialogList->getColor(i), y, dialogList->getLine(i));
        y+=fHeight+4;
    }
    if (currentPlace == 0)
        fb_drawRect(textX,y,textX+textLen, y+fHeight+2, RGB15(27,27,0));
    else
        fb_drawRect(textX,y,textX+textLen, y+fHeight+2, RGB15(26,26,26));
    textInput->rDraw(textX+2, y);
    y+=fHeight+12;
    if (sOK)
    {
        if (currentPlace == 1)
        {
            fb_drawRect(okPosX-2,y-1,okPosX+finalLen+2, y+fHeight+1, RGB15(0,0,0));
            setColor(RGB15(31,31,31));
        }
        else
        {
            fb_drawRect(okPosX-3,y-2,okPosX+finalLen+3, y+fHeight+2, RGB15(0,0,0));
            fb_drawRect(okPosX-2,y-1,okPosX+finalLen+2, y+fHeight+1, RGB15(31,31,31));
            setColor(RGB15(0,0,0));
        }
        if (okLen < finalLen)
            fb_dispString(okPosX+((finalLen-okLen)/2), y, sOK);
        else
            fb_dispString(okPosX, y, sOK);

        setColor(RGB15(0,0,0));
    }
    if (sCancel)
    {
        if (currentPlace == 2)
        {
            fb_drawRect(cancelPosX-2,y-1,cancelPosX+finalLen+2, y+fHeight+1, RGB15(0,0,0));
            setColor(RGB15(31,31,31));
        }
        else
        {
            fb_drawRect(cancelPosX-3,y-2,cancelPosX+finalLen+3, y+fHeight+2, RGB15(0,0,0));
            fb_drawRect(cancelPosX-2,y-1,cancelPosX+finalLen+2, y+fHeight+1, RGB15(31,31,31));
            setColor(RGB15(0,0,0));
        }
        if (cancelLen < finalLen)
            fb_dispString(cancelPosX+((finalLen-cancelLen)/2), y, sCancel);
        else
            fb_dispString(cancelPosX, y, sCancel);
        setColor(RGB15(0,0,0));
    }
    return 2;
}

int rEditBox::rExecute(rKeyboard *kb)
{
    unsigned short x, y;
    int keyHit;
    keyboard = kb;
    hasRun = true;
    keyHit = keyboard->rCheckKey2(EDITBOX_SCREEN, &x, &y);
    keyHit = rParseKey((unsigned char)keyHit);
    if (keyHit == (unsigned char)'\n')
    {
        if (currentPlace == 0)
        {
            currentPlace = 1;
            update = true;
        }
        else
        {
            switch (currentPlace)
            {
                case 1: return D_OK; break;
                case 2: return D_CANCEL; break;
                default: return D_CONTINUE; break;
            }
        }
    }
    return D_CONTINUE;
}

int rEditBox::rParseKey(int key)
{
    int res = 0;
    if ( (key == (unsigned char)rKey::KEY_NAV_DOWN) || (key == (unsigned char)rKey::KEY_NAV_UP) )
    {
        res = key;
    }
    else
    {
        if (currentPlace == 0)
            res = textInput->rPushKeySingleLine(key,textLen);
    }
    if (res == (unsigned char)rKey::KEY_NAV_DOWN)
    {
        update = true;
        currentPlace++;
        if ( (sCancel == NULL) && (currentPlace == 2) )
        {
            currentPlace = 0;
        }
        else if (currentPlace >= 3)
        {
            currentPlace = 0;
        }
    }
    else if (res == (unsigned char)rKey::KEY_NAV_UP)
    {
        update = true;
        currentPlace--;
        if ( (sCancel == NULL) && (currentPlace < 0) )
            currentPlace = 1;
        else if (currentPlace < 0)
            currentPlace = 2;
    }
    return key;
}

char *rEditBox::rGetText()
{
    return textInput->rGetUnformattedBuffer();
}

void rEditBox::rSetText(char *txt)
{
    textInput->rSetBuffer(txt);
    update = true;
}
