#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "nds.h"
#include <../lib/libfb/libcommon.h>
#include "chatlist.h"
#include "textdisplay.h"
#include "../lib/rTOC2/utils.h"
#include "debug.h"

rChatList::rChatList()
{
    height = CHATLIST_DEFAULTHEIGHT;
    width = CHATLIST_DEFAULTWIDTH;
    list = new rTextDisplay(1000, height);
    currentSelect = 0;
    addedUser = false;
}  

rChatList::rChatList(int w, int h)
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
}

rChatList::~rChatList()
{
    if (list)
        delete list;
    currentSelect = 0;
    update = true;
}

bool rChatList::rAddUser(char *user)
{
    int i;
    bool alreadyListed = false;
    addedUser = true;
    //check to see if the user is already in the list
    for (i=0; i<list->getTotalLines(); i++)
    {
        if (strcasecmp(user, list->getLine(i)) == 0) //this is already om the list
        {
            alreadyListed = true;
            break;
        }
    }
    if (!alreadyListed) //need to add this name to the list
    {
        list->addText(user);
//        if (list->getTotalLines() == 1)
            currentSelect = list->getTotalLines()-1;
        update = true;
        return true;
    }
    else //already on the list
    {
        currentSelect = i;
        update = true;
    }
    return false;
}
       
bool rChatList::rKillUser(char *user)
{
    int i;
    //check to see if the user is in the list
    for (i=0; i<list->getTotalLines(); i++)
    {
        if (strcasecmp(user, list->getLine(i)) == 0) //this is already om the list
        {
            list->deleteLine(i);
            currentSelect = 0;
            update = true; 
            return true;
        }
    }
    return false;
}

void rChatList::rKillAllUsers()
{
    delete list;
    list = new rTextDisplay(1000, height);
    update = true;
    currentSelect = 0;
} 

bool rChatList::rScrollUp(int pos)
{
    int res = list->scrollUp(1);
    if (res)
        update = true;
    return res;
}

bool rChatList::rScrollDown(int pos)
{
    int res = list->scrollDown(1);
    if (res)
        update = true;
    return res;
}

char *rChatList::rGetUser(int index)
{
    if (index >= height)
        return NULL;
    if (index < 0)
        return NULL;
    int current = list->getCurrentLine();
    int total = list->getTotalLines();
    int sel = current+index;
    if (sel >= total)
        return NULL;
    return list->getLine(sel);
}

bool rChatList::rIsTalking(char *name)
{
    for (int i=0; i<list->getTotalLines(); i++)
    {
        if (!strcasecmp(name, list->getLine(i)))
            return true;
    }
    return false;
}

void rChatList::rSetFont(unsigned short **f)
{
    list->setFont(f);
}

int rChatList::rNeedsDrawn()
{
    if (!update)
        return 0;
    return 1;
}

int rChatList::rDraw()
{
    if (list->getFont() == NULL)
        return 0;
    if (list->getTotalLines() == 0)
        return 0;
    char userName[100];
    unsigned short ** font = list->getFont();
    int current = list->getCurrentLine();
    if (addedUser)
    {
        addedUser = false;
        if (current > currentSelect)
            list->scrollUp(current-currentSelect);
        else if (current < currentSelect)
            list->scrollDown(currentSelect-current);
    } 
    update = false;
    int ypos = 5;
    int xpos = 10;
    int len = 0;
    int i,j;
    char *tmpUser;
    char Name[100];
    char Alias[100];
    char *dispName;
    bool hasAlias = false;
    current = list->getCurrentLine();
    for ( i=0; i<height; i++)
    {
        len = 0;
        tmpUser = list->getLine(i+current);
        if (tmpUser == NULL)
            break;
        hasAlias = rUtilities::rParseUsername(tmpUser, Name, Alias);
        if (hasAlias)
            dispName = Alias;
        else
            dispName = Name;
        
        for (j=0; j<(int)strlen(dispName); j++)
        {
            len+=font[dispName[j]-32][0];
            if (len <= width)
                userName[j] = dispName[j];
            else
                break;
        }
        userName[j] = '\0';
        if (i+current == currentSelect)
        {
            bg_drawRect(xpos,ypos, 114, ypos+font[0][1], RGB15(0,0,0));
            setColor(RGB15(31,31,31));
            bg_dispString(xpos,ypos,userName);
            rSetColor(RGB15(0,0,0), tmpUser);
        }
        else
        {
            setColor(rGetColor(tmpUser));
            bg_dispString(xpos,ypos,userName);
        }
        setColor(RGB15(0,0,0));
        ypos+=font[0][1]+4;
    }
    return 1;
}
    
bool rChatList::rSelectUser(int pos)
{
    int res = list->getCurrentLine()+pos;
    if (res >= list->getTotalLines())
        return false;
    if (res < 0)
        return false;
    currentSelect = res;
    update = true;
    return true;
}

void rChatList::rSetColor(unsigned short int c, char *name)
{
    int i;
    for (i=0; i<list->getTotalLines(); i++)
    {
        if (strcasecmp(name, list->getLine(i)) == 0) //found it
        {
            list->setColor(c, i);
            update = true;
            break;
        }
    }
    return;
}

unsigned short int rChatList::rGetColor(char *name)
{
    int i;
    for (i=0; i<list->getTotalLines(); i++)
    {
        if (strcasecmp(name, list->getLine(i)) == 0) //found it
        {
            unsigned short int res = 0;
            res =list->getColor(i);
            return res;
        }
    }
    return 0;
}

void rChatList::rSetCurrentUser(int pos)
{
    currentSelect = pos;
    update = true;
}

bool rChatList::rGotoUser(char *name)
{
    int i;
    bool found = false;
    for (i=0; i<list->getTotalLines(); i++)
    {
        if (strcasecmp(name, list->getLine(i)) == 0) //this found it
        {
            found  = true;
            rScrollUp(list->getTotalLines()+10); //scroll to the top
            if (i < height)
                rSelectUser(i);
            else
            {
                int scroll = (i-height)+1;
                rScrollDown(scroll);
                rSelectUser(height-1);
            }
            break;
        }
    }
    return found;
}       
            
