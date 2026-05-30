#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "nds.h"
#include <../lib/libfb/libcommon.h>
#include "buddyview.h"
#include "textdisplay.h"
#include "gba-jpeg-decode.h"
#include "boptions_bin.h"
#include "editbox_bin.h"
#include "globalDefs.h"
#include "debug.h"
#ifdef WIFIDEBUG
#include "../lib/rTOC2/wifidebug.h"
#define WPRINT(format, args...)  rWifiDebug::rPrint("ConvoWindow: %s :" format,__func__, ##args) 
#else
#define WPRINT(format, args...)
#endif

rBuddyView::rBuddyView()
{
    height = BUDDYVIEW_DEFAULTHEIGHT;
    width = BUDDYVIEW_DEFAULTWIDTH;
    list = new rTextDisplay(1000, height);
    currentSelect = 0;
    addedUser = false;
    gID = 0;
    showList = false;

}

rBuddyView::rBuddyView(int w, int h)
{
    if (h < 2) h = 2;
    if (h > 10) h = 10;
    if (w < 15) w = 15;
    height = h;
    width = w;
    list = new rTextDisplay(1000, height);
    currentSelect = 0;
    update = true;
    addedUser = false;
    gID = 0;
    showList = false;
}

rBuddyView::~rBuddyView()
{
    if (list)
        delete list;
    currentSelect = 0;
    update = true;
    gID=0;
}

bool rBuddyView::rAddGroup(char *group, char id)
{
    int i;
    bool found = false;
    for (i=0; i<list->getTotalLines(); i++)
    {
        if ( (list->getType(i) & TYPE_GROUP) == TYPE_GROUP)
        {
            if (!strcasecmp(list->getLine(i), group)) //already there
                return false;
        }
    }
    int current;

    list->addText(group);
    current = list->getTotalLines()-1;
    list->setColor(RGB15(0,0,31), current);
    list->setType(TYPE_GROUP, current);
//    list->setID(gID++, current);
    list->setID(id, current);
    list->setAdditional(0, current);
    //lets sort it!
    int j = 0;
    for (i=0; i<list->getTotalLines()-1; i++)
    {
        if ((list->getType(i) & TYPE_GROUP) == TYPE_GROUP)
        {
            j++;
        }
        if (j-1 == (int) id) //found the spot
        {
            found = true;
            break;
        }
    }
    if (found)
    {
        list->setTmp(list->getTotalLines()-1);
        for (j=list->getTotalLines()-2; j>=i; j--)
        {
           list->swapLine(j+1, j); 
        }
        list->swapTmp(i);
    }         
    return true;
}
 
bool rBuddyView::rDeleteBuddy(char *name)
{
    int j;
    bool found = false;
    for (int i=0; i<list->getTotalLines(); i++)
    {
        if (strcasecmp(list->getLine(i), name) == 0)
        {
            if ( (list->getType(i) & TYPE_BUDDY) == TYPE_BUDDY )
            {
                int groupID = list->getID(i);
                for (j=0; j<list->getTotalLines(); j++)
                {
                    if ( (list->getType(j) & TYPE_GROUP) == TYPE_GROUP )
                    {
                        if (list->getID(j) == groupID)
                        {
                            list->setAdditional(list->getAdditional(j)-1, j);
                            break;
                        }
                    }
                }
                    
                found = true;
                list->deleteLine(i);
                if (i < currentSelect)
                    currentSelect--;
                else if ( (i == currentSelect) && (i > 0) )
                    currentSelect--;
                if (list->getAdditional(j) == 0)
                {
                    list->deleteLine(j);
                    if (j < currentSelect)
                        currentSelect--;
                    else if ( (j == currentSelect) && (j > 0) )
                        currentSelect--;
                }
            }
        }
    }
    if (found)
    {
        update = true;
    }
    return found; 
}

void rBuddyView::rDeleteAllBuddies()
{
    delete list;
    list = new rTextDisplay(1000, height);
    currentSelect = 0;
    addedUser = false;
    gID = 0;
    showList = false;

//    for (int i=0; i<list->getTotalLines(); i++)
//    {
//        list->deleteLine(i);
//    }
//    currentSelect = -1;
//    update = true;
}
    
    
bool rBuddyView::rAddBuddy(char *name, char *group, char *alias)
{
    int i, j;
    char nameAndAlias[200];
    bool foundGroup = false;
    if (alias != NULL)
        sprintf(nameAndAlias, "%s:%s", name, alias);
    else
        strcpy(nameAndAlias, name);
    addedUser = true;
    //check for the group first
    for (i=0; i<list->getTotalLines(); i++)
    {
        if ( (list->getType(i) & TYPE_GROUP) == TYPE_GROUP)
        {
            if (!strcasecmp(list->getLine(i), group)) //already there
            {
                foundGroup = true;
                break;
            }
        }
    }
    if (!foundGroup)
        return false;
    for (j=i+1; j<=(int)list->getAdditional(i)+i; j++)
    {
        if (!strcasecmp(list->getLine(j), nameAndAlias)) //already there
            return false;
    }
    int current;
    int count = 0;
    char *tmp;
    tmp  = list->getLine(i);
    if (alias != NULL)
        list->addText(nameAndAlias);
    else
        list->addText(name);
    tmp  = list->getLine(i);
    current = list->getTotalLines()-1;
    list->setColor(RGB15(0,0,0), current);
    list->setType(TYPE_BUDDY, current);
    list->setID(list->getID(i), current); //assign the group number
    list->setAdditional((unsigned short)strlen(name), current); // length of screen name
    count = (int)list->getAdditional(i); //find out how many buddies are in group i
    count++;
    list->setAdditional((unsigned short)count, i);
    rSortBuddy(current);
    update = true;
    return true;
}

bool rBuddyView::rSortBuddy(int buddy)
{
    int i,j, pos, startLoc;
    i=0; j=0; startLoc=0;
    if ( (list->getType(buddy) & TYPE_BUDDY) != TYPE_BUDDY)
        return false;
    for (i = buddy-1; i>=0; i--)
    {
        if ( (list->getType(i) & TYPE_GROUP)== TYPE_GROUP) //found a group!
        {
            if (list->getID(buddy) == list->getID(i)) //we need not sort this buddy
                return false;
            else
                break;
        }
    }
    //time to sort
    for (i=0; i<list->getTotalLines(); i++)
    {
        if ( ((list->getType(i) & TYPE_GROUP) == TYPE_GROUP) && (list->getID(i) == list->getID(buddy)) ) //found the group
        {
            pos = i+(int)list->getAdditional(i);
            list->setTmp(buddy);
	    for (j = list->getTotalLines()-1; j>=pos; j--)
	    {
                list->swapLine(j, j-1); 
	    }
            list->swapTmp(pos);
            return true;
        }
    }
    return false;
}
                
bool rBuddyView::rScrollUp(int pos)
{
    int res = list->scrollUp(1);
    if (res)
        update = true;
    return res;
}

bool rBuddyView::rScrollDown(int pos)
{
    int res = list->scrollDown(1);
    if (res)
        update = true;
    return res;
}

char *rBuddyView::rGetBuddy(int buddy)
{
    if ( (buddy <0) || (buddy >= list->getTotalLines()) )
        return NULL;
    if ((list->getType(buddy) & TYPE_GROUP) == TYPE_GROUP)
        return NULL;
    return list->getLine(buddy);
}
            
char *rBuddyView::rGetCurrentBuddy()
{
    if ( (list->getType(currentSelect) & TYPE_BUDDY) != TYPE_BUDDY)
        return NULL;
    return list->getLine(currentSelect);
}
    
void rBuddyView::rSetFont(unsigned short **f)
{
    list->setFont(f);
}

bool rBuddyView::rSelectUser(int pos)
{
    if (pos >= list->getTotalLines())
        return false;
    if (pos < 0)
        return false;
    WPRINT("want to move to pos %d", pos); 
    while (pos >= (height+list->getCurrentLine())) //we need to scroll down
    {
        WPRINT("scrolling down");
        if (!rScrollDown(1))
            break;
    }
    while (pos < (list->getCurrentLine()) )
    {
        if (!rScrollUp(1))
            break;
    }
    currentSelect = pos;
    WPRINT("current select = %d", currentSelect);
    update = true;
    return true;
}

int rBuddyView::rNeedsDrawn()
{
    if (!update)
	return 0;
    update = false;
    return 5;
}

int rBuddyView::rDraw()
{
    if (list->getFont() == NULL)
	return 0;
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
    bg_dispSprite(16, 154, (unsigned short int*)boptions_bin, RGB15(31,0,0));
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
        if (tmpName[list->getAdditional(i+current)] == ':')
            nameStart = list->getAdditional(i+current)+1;
        else
            nameStart = 0;
        int index = 0;
        for (j=nameStart; j<(int)strlen(tmpName); j++, index++)
        {
            if (tmpName[j] == ':')
                break;
            len+=font[tmpName[j]-32][0];
            if (len <= width)
                name[index] = tmpName[j];
            else
                break;
        }
	name[index] = '\0';
        if ( (list->getType(i+current) & BUDDYVIEW_BUDDY_AWAY) == BUDDYVIEW_BUDDY_AWAY)
            strcat(name, " (Away)");
        if ( (list->getType(i+current) & BUDDYVIEW_BUDDY_IDLE) == BUDDYVIEW_BUDDY_IDLE)
            strcat(name, " (Idle)");
        
        if (i+current == currentSelect)
        {
            bg_drawRect(22,ypos, 219, ypos+font[0][1]+1, RGB15(0,0,0));
            setColor(RGB15(31,31,31));
	    if ((list->getType(i+current) & TYPE_GROUP) == TYPE_GROUP)
            {
                sprintf(name, "%s (%d)",name, list->getAdditional(i+current));
		xpos = 22;
            }
	    else
		xpos = 28;
            bg_dispString(xpos,ypos,name);
            setColor(RGB15(0,0,0));
        }
        else
        {
	    if ( (list->getType(i+current) & TYPE_GROUP) == TYPE_GROUP)
	    {
		setColor(RGB15(0,0,31));
                sprintf(name, "%s (%d)",name, list->getAdditional(i+current));
		xpos = 22;
	    }
	    else
	    {
		setColor(RGB15(0,0,0));
		xpos = 28;
	    }
            bg_dispString(xpos,ypos,name);
        }
        setColor(RGB15(0,0,0));
        ypos+=font[0][1]+1;
    }
    return 5;
}

void rBuddyView::rToggleShow()
{
    showList = !showList;
    if (showList)
	update = true;
}

int rBuddyView::rPushKey(int x, int y)
{
    int i = 0;
    int factor = 0;
    if (INSIDE(x, y, 222, 10, 238, 28)) //scroll up
    {
        rScrollUp(1);
        return rKey::KEY_BUDDYLIST_SCROLLUP; 
    }
    else if (INSIDE(x, y, 222, 136, 238, 153)) //scroll down
    {
	rScrollDown(1);
        return rKey::KEY_BUDDYLIST_SCROLLDOWN; 
    }
    else if (INSIDE(x, y, 98, 154, 139, 175)) //SEND IM
    {
        return rKey::KEY_BUDDY_IM;
    }
    else if (INSIDE(x, y, 58, 154, 95, 175)) //ADD BUDDY
    {
        return rKey::KEY_BUDDY_ADD;
    }
    else if (INSIDE(x, y, 18, 154, 55, 175)) //GET INFO
    {
        return rKey::KEY_BUDDY_INFO;
    }
    else
    {
        unsigned short int **font = list->getFont();
        bool found = false;
        factor = 14;
        for (i=0; i<height; i++)
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
            return currentSelect;
        }
    }
    return 0;
} 

bool rBuddyView::rSetAway(char *name, bool on)
{
    bool found = false;
    char type = 0;
    for (int i=0; i<list->getTotalLines(); i++)
    {
        if (strcasecmp(list->getLine(i), name) == 0)
        {
            if ( (list->getType(i) & TYPE_BUDDY) == TYPE_BUDDY )
            {
                found = true;
                type = list->getType(i);
                if (on)
                    type |= BUDDYVIEW_BUDDY_AWAY;
                else
                    type &= ~BUDDYVIEW_BUDDY_AWAY;
                list->setType(type, i);
                update = true;
            }
        }
    }
    return found;
}

int rBuddyView::rGetStats(char *name)
{
    bool found = false;
    char type = 0;
    for (int i=0; i<list->getTotalLines(); i++)
    {
        if (strcasecmp(list->getLine(i), name) == 0)
        {
            if ( (list->getType(i) & TYPE_BUDDY) == TYPE_BUDDY )
            {
                found = true; //must be online
                type = list->getType(i);
                type |= BUDDYVIEW_BUDDY_ONLINE;
                return type;
            }
        }
    }
    return BUDDYVIEW_BUDDY_OFFLINE;
}

bool rBuddyView::rSetIdle(char *name, bool on)
{
    bool found = false;
    char type = 0;
    for (int i=0; i<list->getTotalLines(); i++)
    {
        if (strcasecmp(list->getLine(i), name) == 0)
        {
            if ( (list->getType(i) & TYPE_BUDDY) == TYPE_BUDDY )
            {
                found = true;
                type = list->getType(i);
                if (on)
                    type |= BUDDYVIEW_BUDDY_IDLE;
                else
                    type &= ~BUDDYVIEW_BUDDY_IDLE;
                list->setType(type, i);
                update = true;
            }
        }
    }
    return found;
}

char *rBuddyView::rGetBuddyAlias(char *uname)
{
    char *sn;
    int j = 0;
    for (int i=0; i<list->getTotalLines(); i++)
    {
        if ( (list->getType(i) & TYPE_BUDDY) == TYPE_BUDDY)
        {
            j = 0;
            sn = list->getLine(i);
            while (1)
            {
                if (sn[j] == ':')
                {
                    sn[j] = '\0';
                    if (!strcasecmp(uname, sn))
                    {
                        sn[j] = ':';
                        return sn;
                    }
                    else
                    {
                        sn[j] = ':';
                        break;
                    }
                }
                if (sn[j] == '\0')
                {
                    if (!strcasecmp(uname, sn))
                    {
                        return sn;
                    }
                    else
                    {
                        break;
                    }
                }
                j++;
            }
        }
    }
    return uname;
}

