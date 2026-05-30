#include <string.h>
#include <stdio.h>
#include "nds.h"
#include <../lib/libfb/libcommon.h>
#include <stdarg.h>
#include "dialog.h"
#include "globalDefs.h"
#ifdef USEWIFI
#include "wificonnect.h"
#define WPRINT(format, args...)  rWifiDebug::rPrint("Template: %s :" format,__func__, ##args)
#else
#define WPRINT(format, args...)
#endif
#include "../lib/rTOC2/wifidebug.h"
#include "debug.h"

rDialog::rDialog() {}
rDialog::rDialog(char *theText, char *okText, char *cancelText, int x, int y, int id, unsigned short int ** fnt, bool derived)
{
    hasRun = false;
    rSetFont(fnt);
    currentSelect = -1;
    dialogID = id;
    sOK = sCancel = NULL;
    if (okText != NULL)
    {
        if (strlen(okText) > 0)
        {
            sOK = new char[strlen(okText)+1];
            strcpy(sOK, okText);
        }
        else
            sOK = NULL; 
    }
    if (cancelText != NULL)
    {
        if (strlen(cancelText) > 0)
        {
            sCancel = new char[strlen(cancelText)+1];
            strcpy(sCancel, cancelText);
        }
        else
            sCancel = NULL;
    }
    keyboard = NULL;
    dialogList = new rTextDisplay(200, 6);
    if (theText)
    {
        dialogList->setFont(font);
        dialogList->addText(theText);
    }
    isActive = false;
    update = true;
    delayKill = 0;
    drawX = x; 
    drawY = y;
    returnValue = D_CONTINUE;
    if (!derived)
    {
        rCalcPos();
    }
} 

rDialog::~rDialog() 
{
    keyboard = NULL;
    if (dialogList)
        delete dialogList;
    if (sOK)
        delete [] sOK;
    if (sCancel)
        delete [] sCancel;
    font = NULL;
}

void rDialog::rCalcPos()
{
    int i, j;
    int len = 0;
    int fHeight;
    int boxHeight;
    fHeight = font[0][1]+1;
    boxHeight = ( (dialogList->getTotalLines() * (fHeight+4)) )+4+30;
    int y = drawY;
    if (y+boxHeight >= 192)
    {
        y = boxHeight-10;
    }
    y+=4;
    y+=(fHeight+4)*dialogList->getTotalLines();
    y+=6;
    finalLen = -1;
    okPosY = -1;
    okPosY = y;
    okPosX = -1;
    cancelPosX = -1;
    okLen = -1;
    cancelLen = -1;
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

int rDialog::rNeedsDrawn()
{
    if (!update)
	return 0;
    update = false;
    return 5;
}

int rDialog::rDraw()
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
    int boxHeight = ( (dialogList->getTotalLines() * (fHeight+4)) )+4+30;
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
		bg_setPixel(j,i,RGB15(0,0,0));
	}
	else
	{
	    for (j=1; j<256; j+=2)
		bg_setPixel(j,i,RGB15(0,0,0));
	}
    }
    bg_drawRect(x,y,256-x, y+boxHeight, RGB15(0,0,0));
    bg_drawRect(x+2,y+2,256-x-2, y+boxHeight-2, RGB15(31,31,31));
    
    y = y+4;
    for (i=0; i<dialogList->getTotalLines(); i++)
    {
        bg_dispString(dialogList->getColor(i), y, dialogList->getLine(i));
        y+=fHeight+4;
    }
    y+=6;

    if (sOK)
    {
        if (currentSelect == 0)
        {
            bg_drawRect(okPosX-2,y-1,okPosX+finalLen+2, y+fHeight+1, RGB15(0,0,0));
            setColor(RGB15(31,31,31));
        }
        else
        {
            bg_drawRect(okPosX-3,y-2,okPosX+finalLen+3, y+fHeight+2, RGB15(0,0,0));
            bg_drawRect(okPosX-2,y-1,okPosX+finalLen+2, y+fHeight+1, RGB15(31,31,31));
            setColor(RGB15(0,0,0));
        }
        if (okLen < finalLen)
            bg_dispString(okPosX+((finalLen-okLen)/2), y, sOK);
        else
            bg_dispString(okPosX, y, sOK);

        setColor(RGB15(0,0,0));
    }
    if (sCancel)
    {
        if (currentSelect == 1)
        {
            bg_drawRect(cancelPosX-2,y-1,cancelPosX+finalLen+2, y+fHeight+1, RGB15(0,0,0));
            setColor(RGB15(31,31,31));
        }
        else
        {
            bg_drawRect(cancelPosX-3,y-2,cancelPosX+finalLen+3, y+fHeight+2, RGB15(0,0,0));
            bg_drawRect(cancelPosX-2,y-1,cancelPosX+finalLen+2, y+fHeight+1, RGB15(31,31,31));
            setColor(RGB15(0,0,0));
        }
        if (cancelLen < finalLen)
            bg_dispString(cancelPosX+((finalLen-cancelLen)/2), y, sCancel);
        else
            bg_dispString(cancelPosX, y, sCancel);
        setColor(RGB15(0,0,0));
    }
    return 5;
}

int rDialog::rExecute(rKeyboard *kb)
{
    unsigned short x, y;
    int keyHit;
    keyboard = kb;
    hasRun = true;
    bool pickup = true;
    short int id = -1;
    x = y = -1;
    keyHit = keyboard->rCheckKey2(DIALOG_SCREEN, &x, &y, &pickup);
    if (returnValue == D_CONTINUE)
        id = rParseKey((unsigned char)keyHit, (unsigned short)x, (unsigned short) y);
    if (id != D_CONTINUE)
    {
        returnValue = id;
        update = true;
    }
    if ( (returnValue != D_CONTINUE) && (delayKill >= DIALOG_DELAY))
    {
        update = false;
        return returnValue;
    }
    else if ( (returnValue != D_CONTINUE) && (delayKill < DIALOG_DELAY))
    {
        delayKill++;
        update = true;
    }
    return D_CONTINUE;
}

int rDialog::rParseKey(int type, int px, int py)
{
    if (font == NULL)
        return D_ERROR;
    int fHeight = font[0][1]+1;
    if (sOK)
    {
        if ( (px >= okPosX) && (px <= okPosX+6+finalLen) )
        {
            if ( (py >= okPosY-1) && (py <= okPosY+1+fHeight) )
            {
                currentSelect = 0;
                return D_OK;
            }
        }
    }
    if (sCancel)
    {
        if ( (px >= cancelPosX) && (px <= cancelPosX+6+finalLen) )
        {
            if ( (py >= okPosY-1) && (py <= okPosY+1+fHeight) )
            {
                currentSelect = 1;
                return D_CANCEL;
            }
        }
    }
    return D_CONTINUE;        
}
