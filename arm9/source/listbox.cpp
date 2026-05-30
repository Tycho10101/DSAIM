#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "nds.h"
#include <../lib/libfb/libcommon.h>
#include "listbox.h"
#include "textdisplay.h"
#include "keyboard.h"
#include "gba-jpeg-decode.h"
#include "editbox_bin.h"
#include "globalDefs.h"
#include "menudefs.h"
#ifdef WIFIDEBUG
#include "../lib/rTOC2/wifidebug.h"
#define WPRINT(format, args...)  rWifiDebug::rPrint("Template: %s :" format,__func__, ##args)
#else
#define WPRINT(format, args...)
#endif

rListBox::rListBox(int w, int h, int id, unsigned short int **fnt)
{
    if (h < 2) h = 2;
    if (h > 10) h = 10;
    if (w < 15) w = 15;
    hasRun = false;
    WPRINT("creating listbox");
    height = h;
    width = w;
    list = new rTextDisplay(width, height);
    delayKill = 0;
    currentSelect = 0;
    addedString = addedData = false;
    showList = false;
    rSetFont(fnt);
    dialogID = id;
    returnValue = D_CONTINUE;
    buttonHit = 0;
    populateFunc = NULL;

    int xLen = 0;
    char *OKText = OKCancelText[0];
    for (int i=0; i<(int)strlen(OKText); i++)
    {
        xLen += fnt[OKText[i]-32][0]+1;
    }
    okX = 16 + ((56-16)/2) - (xLen/2);
    okY = 154 + ((175-154)/2) - (fnt[OKText[1]][1]/2);

    OKText = OKCancelText[1];
    xLen = 0;
    for (int i=0; i<(int)strlen(OKText); i++)
    {
        xLen += fnt[OKText[i]-32][0]+1;
    }
    cancelX = 57 + ((97-57)/2) - (xLen/2);
    cancelY = 154 + ((175-154)/2) - (fnt[OKText[1]][1]/2);
    WPRINT("done making listbox");

}

rListBox::~rListBox()
{
    if (list)
        delete list;
    currentSelect = 0;
    update = true;
}

int rListBox::rAddString(char *str)
{
    update = true;
    return list->addText(str);
}

bool rListBox::rInsertData(int pos, void *data)
{
    if (pos >= list->getTotalLines())
        return false;
    else
        list->setExtra((char*)data, pos);
    return true;
}

bool rListBox::rDeleteString(int pos)
{
    if (pos >= list->getTotalLines())
        return false;
    else
    {
        list->deleteLine(pos);
        update = true;
    }
    return true;
}

void rListBox::rDeleteAll()
{
    delete list;
    list = new rTextDisplay(1000, height);
    list->setFont(font);
    currentSelect = 0;
    addedString = addedData = false;
    showList = false;
}

bool rListBox::rScrollUp(int pos)
{
    int res = list->scrollUp(1);
    if (res)
        update = true;
    return res;
}

bool rListBox::rScrollDown(int pos)
{
    int res = list->scrollDown(1);
    if (res)
        update = true;
    return res;
}

char *rListBox::rGetText(int pos)
{
    if (pos >= list->getTotalLines())
        return NULL;
    else
        return list->getLine(pos);
}

void *rListBox::rGetData(int pos)
{
    if (pos >= list->getTotalLines())
        return NULL;
    else
        return list->getExtra(pos);
}

char *rListBox::rGetCurrentText()
{
    if (currentSelect >= list->getTotalLines())
        return NULL;
    return list->getLine(currentSelect);
}

void *rListBox::rGetCurrentData()
{
    if (currentSelect >= list->getTotalLines())
        return NULL;
    return list->getExtra(currentSelect);
}

void rListBox::rSetFont(unsigned short** fnt)
{
    font = fnt;
    list->setFont(font);
}
    
int rListBox::rParseKey(int key, int x, int y)
{
    int i = 0;
    int factor = 0;
    if (INSIDE(x, y, 222, 10, 238, 28)) //scroll up
    {
        rScrollUp(1);
        return D_CONTINUE;
    }
    else if (INSIDE(x, y, 222, 136, 238, 153)) //scroll down
    {
	rScrollDown(1);
        return D_CONTINUE;
    }
    else if (INSIDE(x, y, 16, 154, 56, 175)) //HIT OK
    {
        return D_OK;
    }
    else if (INSIDE(x, y, 57, 154, 97, 175)) //HIT CANCEL
    {
        return D_CANCEL;
    }
    else
    {
        unsigned short int **font = list->getFont();
        bool found = false;
        factor = 14;
        for (i=0; i<list->getTotalLines(); i++)
        {
            if (INSIDE(x, y, 22, factor, 221, factor+font[0][1]+1)) //slot
            {
                currentSelect = list->getCurrentLine()+i;
                found = true;
                break;
            }
            factor +=13;
        }
        if (found)
        {
            update = true;
            return D_CONTINUE;
        }
    }
    return D_CONTINUE;
} 

bool rListBox::rSelectPos(int pos, bool scroll)
{
    if (pos >= list->getTotalLines())
        return false;
    if (pos < 0)
        return false;
    if (scroll)
    {
        while (pos >= (height+list->getCurrentLine())) //we need to scroll down
        {
            if (!rScrollDown(1))
                break;
        }
        while (pos < (list->getCurrentLine()) )
        {
            if (!rScrollUp(1))
                break;
        }
    }
    currentSelect = pos;
    update = true;
    return true;
}

void rListBox::rToggleShow()
{
    showList = !showList;
    if (showList)
	update = true;
}

int rListBox::rNeedsDrawn()
{
    if (!update)
	return 0;
    update = false;
    return 5;
}

int rListBox::rDraw()
{
    if (list->getFont() == NULL)
	return 0;
    if (currentSelect > list->getTotalLines())
        currentSelect = 0;
    update = false;
    int i, j;
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
    bg_dispSprite(16, 10, (unsigned short int*)editbox_bin, RGB15(31,0,0));
    bg_drawRect(16,154, 56, 175, RGB15(0,0,0)); //OK
    bg_drawRect(57,154, 97, 175, RGB15(0,0,0)); //Cancel
    switch(buttonHit)
    {
        case D_OK: //ok hit
            bg_drawRect(17, 155, 55, 174, RGB15(0,0,0));
            setColor(RGB15(31,31,31));
            bg_dispString(okX, okY, OKCancelText[0]);
            setColor(RGB15(0,0,0));

            bg_drawRect(58, 155, 96, 174, RGB15(31,31,31));
            bg_dispString(cancelX, cancelY, OKCancelText[1]);
            break;
        case D_CANCEL: //cancel hit
            bg_drawRect(58, 155, 96, 174, RGB15(0,0,0));
            setColor(RGB15(31,31,31));
            bg_dispString(cancelX, cancelY, OKCancelText[1]);
            setColor(RGB15(0,0,0));

            bg_drawRect(17, 155, 55, 174, RGB15(31,31,31));
            bg_dispString(okX, okY, OKCancelText[0]);
            break;
        default: //none hit
            bg_drawRect(17, 155, 55, 174, RGB15(31,31,31));
            bg_dispString(okX, okY, OKCancelText[0]);
            bg_drawRect(58, 155, 96, 174, RGB15(31,31,31));
            bg_dispString(cancelX, cancelY, OKCancelText[1]);
            break;
    }
    if (list->getFont() == NULL)
        return 5;
    if (list->getTotalLines() == 0)
	return 5;
    char name[100];
    unsigned short int **font = list->getFont();
    int current = list->getCurrentLine();
    char *tmpName;
    int ypos = 14;
    int xpos = 22;
    int len=0;
    int nameStart = 0;
    
    for (i=0; i<height; i++)
    {
	len = 0;
	tmpName = list->getLine(i+current);
	if (tmpName == NULL)
	    break;
        int index = 0;
        for (j=nameStart; j<(int)strlen(tmpName); j++, index++)
        {
            len+=font[tmpName[j]-32][0];
            if (len <= width)
                name[index] = tmpName[j];
            else
                break;
        }
	name[index] = '\0';
        
        if (i+current == currentSelect)
        {
            bg_drawRect(22,ypos, 219, ypos+font[0][1]+1, RGB15(0,0,0));
            setColor(RGB15(31,31,31));
	    xpos = 28;
            bg_dispString(xpos,ypos,name);
            setColor(RGB15(0,0,0));
        }
        else
        {
	    setColor(RGB15(0,0,0));
	    xpos = 28;
            bg_dispString(xpos,ypos,name);
        }
        setColor(RGB15(0,0,0));
        ypos+=font[0][1]+1;
    }
    return 5;
}
int rListBox::rExecute(rKeyboard *kb)
{
    unsigned short x, y;
    int keyHit;
    keyboard = kb;
    hasRun = true;
    bool pickup = true;
    short int id = -1;
    x = y = -1;
    WPRINT("executing the listbox");
    keyHit = keyboard->rCheckKey2(LISTBOX_SCREEN, &x, &y, &pickup);
    if (returnValue == D_CONTINUE)
    {
        if (populateFunc)
        {
            if (currentCycle == 0)
            {
                WPRINT("running populate");	
                (*populateFunc)((void*)list);
            }
            currentCycle++;
            currentCycle %= totalCycles;
        }
        id = rParseKey((unsigned char)keyHit, (unsigned short)x, (unsigned short) y);
    }
    if (id >= 0)
    {
        returnValue = id;
        update = true;
        buttonHit = id;
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

