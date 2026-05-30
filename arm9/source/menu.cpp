#include <string.h>
#include <stdio.h>
#include "nds.h"
#include <../lib/libfb/libcommon.h>
#include <stdarg.h>
#include "menu.h"
#include "globalDefs.h"
#include "wificonnect.h"
#include "debug.h"
#ifdef WIFIDEBUG
#include "../lib/rTOC2/wifidebug.h"
#define WPRINT(format, args...)  rWifiDebug::rPrint("Keyboard %s :" format,__func__, ##args)
#else
#define WPRINT(format, args...)
#endif

rMenu::rMenu() {}
rMenu::rMenu(char *theTitle, char *theDescription, int x, int y, int id, unsigned short int ** fnt)
{
    hasRun = false;
    rSetFont(fnt);
    title = desc = NULL;
    menuID = id;
    if (theTitle != NULL)
    {
        if (strlen(theTitle) > 0)
        {
            title = new char[strlen(theTitle)+1];
            strcpy(title, theTitle);
        }
        else
            title = NULL; 
    }
    if (theDescription != NULL)
    {
        if (strlen(theDescription) > 0)
        {
            desc = new char[strlen(theDescription)+1];
            strcpy(desc, theDescription);
        }
        else
            desc = NULL;
    }
    keyboard = NULL;
    menuList = new rTextDisplay(1000, 6); 
    status[0] = '\0';
    tx = dx = 0;
    isActive = false;
    currentSelect = -1;
    update = true;
    delayKill = 0;
    drawX = x; 
    drawY = y;
    returnValue = D_CONTINUE;
    rCalcPos();
} 

rMenu::~rMenu() 
{
    keyboard = NULL;
    if (menuList)
        delete menuList;
    if (title)
        delete [] title;
    if (desc)
        delete [] desc;
    font = NULL;
}

void rMenu::rAddOption(char *name, int id)
{
    if (font == NULL)
        return;
    menuList->addText(name);
    menuList->setAdditional((unsigned short int)id, menuList->getTotalLines()-1);
    rCalcPos();
    update = true; 
}

void rMenu::rCalcPos()
{
    int i, j;
    int len = 0;
    if (title)
    {
        len = 0;
        for (i=0; i<(int)strlen(title); i++)
            len += font[title[i]-32][0]+1;
        tx = (256-len)/2;
    }
    if (desc)
    {
        len = 0;
        for (i=0; i<(int)strlen(desc); i++)
            len += font[desc[i]-32][0]+1;
        dx = (256-len)/2;
    }
    char *pch;
    for (j=0; j<menuList->getTotalLines(); j++)
    {
        len = 0;
        pch = menuList->getLine(j);
        for (i=0; i<(int)strlen(pch); i++)
            len += font[pch[i]-32][0]+1;
        menuList->setColor((short int)(((256-4)-(len))/2), j); //use color to store xPosition;
    }
}

int rMenu::rNeedsDrawn()
{
    if (!update)
	return 0;
    update = false;
    return 5;
}

int rMenu::rDraw()
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
    int boxHeight = ( (menuList->getTotalLines() * (fHeight+4)) + (fHeight+4)*2 )+4;
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
    if (title)
    {
        bg_dispString(tx, y+4, title);
        bg_dispString(tx+1, y+4, title);
    }
    if (desc)
        bg_dispString(dx, y+4+fHeight, desc);
    bg_drawRect(x,y+4+(fHeight*2)+2,256-x, y+4+(fHeight*2)+3, RGB15(0,0,0));
    y = y+10+(fHeight*2);
    for (i=0; i<menuList->getTotalLines(); i++)
    {
        if (menuList->getCurrentLine()+i == currentSelect)
        {
            if (currentSelect == 0)
                bg_drawRect(x,y-1,256-x, y+fHeight+2, RGB15(0,0,0));
            else
                bg_drawRect(x,y-2,256-x, y+fHeight+2, RGB15(0,0,0));
            setColor(RGB15(31,31,31));
        }
        else
            setColor(RGB15(0,0,0));
        bg_dispString(menuList->getColor(i), y, menuList->getLine(i));
        y+=fHeight+4;
    }
    return 5;
}

int rMenu::rExecute(rKeyboard *kb)
{
    unsigned short x, y;
    int keyHit;
    keyboard = kb;
    hasRun =  true;
    bool pickup = true;
    short int id = -1;
    {
        x = y = -1;
        keyHit = keyboard->rCheckKey2(MENU_SCREEN, &x, &y, &pickup);
        if (returnValue == D_CONTINUE)
            id = rParseKey((unsigned char)keyHit, (unsigned short)x, (unsigned short) y);
        if (id != D_CONTINUE)
        {
            returnValue = id;
            update = true;
        }
        if ( (returnValue != D_CONTINUE) && (delayKill >= DIALOG_DELAY))
            return returnValue;
        else if ( (returnValue != D_CONTINUE) && (delayKill < DIALOG_DELAY))
        {
            delayKill++;
            update = true;
        }
    }
    return D_CONTINUE;
}

int rMenu::rParseKey(int type, int px, int py)
{
    if (font == NULL)
        return D_ERROR;
    int i;
    int y;
    int fHeight;
    int boxHeight;
    fHeight = font[0][1]+1;
    boxHeight = ( (menuList->getTotalLines() * (fHeight+4)) + (fHeight+4)*2 )+4;
    y = drawY;
    if (y+boxHeight >= 192)
    {
        y = boxHeight-10;
    }
    y+=4;
    if ( (px <= drawX) || (px >= (256-drawX)) )
        return D_CONTINUE;
    if ( (py <= drawY) || (py >= (drawY+boxHeight)) )
        return D_CONTINUE;
    y = drawY;
    if (y+boxHeight >= 192)
    {
        y = boxHeight-10;
    }
    y = y+10+(fHeight*2);
    for (i=0; i<menuList->getTotalLines(); i++)
    {
        if ( (py >= y-2) && (py < y+fHeight+2) )
        {
            currentSelect = i;
            return menuList->getAdditional(i);
        }
        y+=fHeight+4;
    }
    return D_CONTINUE;
}

    


        
    

    
    

