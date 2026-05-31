#include <stdio.h>
#include <string.h>
#include "nds.h"
#include "globalDefs.h"
#include <../lib/libfb/libcommon.h>
#include "convo.h"
#include "commonObjs.h"
#include "../lib/rTOC2/buddy.h"
#include "menudefs.h"
#ifdef WIFIDEBUG
#include "../lib/rTOC2/wifidebug.h"
#endif
#include "../lib/rTOC2/utils.h"
#include "textdisplay.h"
#include "debug.h"

#define DBG(x)  char zbuf[255]; sprintf(zbuf, "total %d", x); setFont((uint16**)font_arial_8); setColor(RGB15(31,0,0)); fb_dispString(17,30,zbuf); fb_swapBuffers()
#ifdef WIFIDEBUG
#define WPRINT(format, args...)  rWifiDebug::rPrint("ConvoWindow: %s :" format,__func__, ##args) 
#else
#define WPRINT(format, args...)
#endif

rConversationWindow *convoInstance;

rConversationWindow::rConversationWindow()
{
    currentConversation = -1;
    input = NULL;
    currentStatus = STATUS_NOTOC;
    //WPRINT("Creating textdisplay");
    convos = new rTextDisplay*[MAXCONVO];
    for (int i=0; i<MAXCONVO; i++)
    {
        convos[i] = NULL;
        chatRoomNames[i][0] = '\0';
        chatRoomIDs[i] = -1;
        chatRoomMode[i] = false;
    }
    //WPRINT("Creating input");
    input = new rUserInput();
    //WPRINT("Creating chatlist");
    chatList = new rChatList();
    totalConversations = 0;
    maxConversations = MAXCONVO;
    activeText = NULL;
    memset(username, 0, 50);
    memset(password, 0, 50);
    font = NULL;
    //WPRINT("Creating buddyview");
    buddylist = new rBuddyView();
    update = true;
    convoInstance = this;
    //WPRINT("Done!");

#ifdef USEDEBUG
    int stats = 6;
    rGetBuddyList()->rAddGroup("Friends", 0);
    rGetBuddyList()->rAddBuddy("JIM", "Friends");
#endif
    
//    client = NULL;
}

rConversationWindow::rConversationWindow(int max)
{
    currentConversation = -1;
    input = NULL;
    //WPRINT("Creating textdisplay");
    convos = new rTextDisplay*[max];
    for (int i=0; i<max; i++)
    {
        convos[i] = NULL;
        chatRoomNames[i][0] = '\0';
        chatRoomIDs[i] = -1;
        chatRoomMode[i] = false;
    }
    //WPRINT("Creating input");
    input = new rUserInput();
    //WPRINT("Creating chatlist");
    chatList = new rChatList();
    totalConversations = 0;
    maxConversations = max;
    memset(username, 0, 50);
    memset(password, 0, 50);
    activeText = NULL;
    font = NULL;
    //WPRINT("Creating buddyview");
    buddylist = new rBuddyView(); 
    update = true; 
    convoInstance = this; 
    //WPRINT("Done!");
//    client = NULL;
}

rConversationWindow::~rConversationWindow()
{
    rKillAllConversations();
    delete [] convos;
    delete input;
    delete chatList;
    delete buddylist;
}

int rConversationWindow::rFindConversation(const char *name)
{
    if (name == NULL)
        return -1;
    for (int i=0; i<totalConversations; i++)
    {
        if (convos[i] != NULL && strcasecmp(name, convos[i]->getID()) == 0)
            return i;
    }
    return -1;
}

bool rConversationWindow::rIsChatRoomConversation(int index)
{
    if (index < 0 || index >= maxConversations)
        return false;
    return chatRoomMode[index];
}

int rConversationWindow::rGetRoomID(int index)
{
    if (!rIsChatRoomConversation(index))
        return -1;
    return chatRoomIDs[index];
}

void rConversationWindow::rAddChatRoom(char *roomName, int roomID)
{
    char displayName[128];
    if (roomName == NULL || roomName[0] == '\0')
        return;

    snprintf(displayName, sizeof(displayName), "#%s", roomName);
    const int existing = rFindConversation(displayName);
    if (existing >= 0)
    {
        chatRoomMode[existing] = true;
        if (roomID >= 0)
            chatRoomIDs[existing] = roomID;
        strncpy(chatRoomNames[existing], roomName, sizeof(chatRoomNames[existing]) - 1);
        chatRoomNames[existing][sizeof(chatRoomNames[existing]) - 1] = '\0';
        update = true;
        return;
    }

    for (int i=0; i<maxConversations; i++)
    {
        if (convos[i] == NULL)
        {
            convos[i] = new rTextDisplay();
            convos[i]->setID(displayName);
            convos[i]->setFont((uint16**)font);
            chatRoomMode[i] = true;
            chatRoomIDs[i] = roomID;
            strncpy(chatRoomNames[i], roomName, sizeof(chatRoomNames[i]) - 1);
            chatRoomNames[i][sizeof(chatRoomNames[i]) - 1] = '\0';
            chatList->rAddUser(displayName);
            chatList->rSetColor(0, displayName);
            totalConversations++;
            rSetActive(displayName);
            update = true;
            return;
        }
    }
}

void rConversationWindow::rAddChatRoomMessage(char *roomName, char *who, char *msg)
{
    char displayName[128];
    char sender[200];
    char *sMsg;
    char *line;
    int lineLen;
    int idx;
    if (roomName == NULL || who == NULL || msg == NULL)
        return;

    snprintf(displayName, sizeof(displayName), "#%s", roomName);
    idx = rFindConversation(displayName);
    if (idx < 0)
    {
        rAddChatRoom(roomName, -1);
        idx = rFindConversation(displayName);
    }

    sMsg = new char[strlen(msg) + 1];
    strcpy(sMsg, msg);
    convoInstance->rParseHTML(sMsg);

    strncpy(sender, who, sizeof(sender) - 1);
    sender[sizeof(sender) - 1] = '\0';

    lineLen = strlen(sender) + strlen(sMsg) + 10;
    line = new char[lineLen];
    if (!rUtilities::rNormalizeCompare(sender, username))
        sprintf(line, "%c%s: %s", COLOR_BLUE, sender, sMsg);
    else
        sprintf(line, "%c%s: %s", COLOR_RED, sender, sMsg);

    if (idx >= 0 && convos[idx] != NULL)
    {
        convos[idx]->addText(line);
        if (idx != currentConversation)
            chatList->rSetColor(RGB15(31,0,0), displayName);
        else
            chatList->rSetColor(0, displayName);
        update = true;
    }
    delete []line;
    delete []sMsg;
}

int rConversationWindow::rAddMessage(char *who, char *msg, bool away)
{
    bool found = false;
    int i;
    char userNoAlias[100];
    char alias[100];
    bool hasAlias;
    hasAlias = rUtilities::rParseUsername(who, userNoAlias, alias);
    for (i=0; i<totalConversations; i++)
    {
        if (strcasecmp(who, convos[i]->getID()) == 0)
        {
            if (msg == NULL) //user is adding the message after hitting send
            {
                msg = input->rFormat();
                if (strlen(msg) > 0 && !chatRoomMode[i])
                {
                    char *nameAndMsg = new char[strlen(username)+strlen(msg)+10];
                    sprintf(nameAndMsg, "%c%s: %s", COLOR_BLUE, username, msg); 
                    convos[i]->addText(nameAndMsg);
                    delete []nameAndMsg;
                }
                if (*client != NULL)
                {
                    if (strlen(msg) > 0)
                    {
                        if (chatRoomMode[i])
                            (*client)->rSendChatRoom(chatRoomIDs[i], msg);
                        else
                            (*client)->rSendIM(userNoAlias, msg);
                    }
                }
                delete []msg;
                convos[i]->clearInput();
            }
            else if (msg[0] == '\0') //empty message, usually after selecting user from bubdy list
            {
                chatList->rGotoUser(who);
                rSetActive(who);
                update = true;
            } 
            else
            {
                int addition = 10;
                char *autoResponse ="Auto Response";
                if (away) addition+=strlen(autoResponse)+1;
                char *nameAndMsg;
                if (hasAlias)
                {
                    nameAndMsg = new char[strlen(alias)+strlen(msg)+addition];
                    if (away) 
                        sprintf(nameAndMsg, "%c%s[%s]:\n%s", COLOR_RED, alias, autoResponse, msg); 
                    else
                        sprintf(nameAndMsg, "%c%s: %s", COLOR_RED, alias, msg); 
                }
                else
                {
                    nameAndMsg = new char[strlen(userNoAlias)+strlen(msg)+addition];
                    if (away) 
                        sprintf(nameAndMsg, "%c%s[%s]:\n%s", COLOR_RED, userNoAlias, autoResponse, msg); 
                    else
                        sprintf(nameAndMsg, "%c%s: %s", COLOR_RED, userNoAlias, msg); 
                }    
                convos[i]->addText(nameAndMsg);
                chatList->rSetColor(RGB15(31,0,0), who);
                delete []nameAndMsg;
            }
            found = true;
            break;
        }
    }
    if (!found)
    {
        for (i=0; i<maxConversations; i++)
        {
            if (convos[i] == NULL)
            {
                convos[i] = new rTextDisplay();
                convos[i]->setID(who);
                convos[i]->setFont((uint16**)font);
                chatRoomMode[i] = false;
                chatRoomIDs[i] = -1;
                chatRoomNames[i][0] = '\0';
                chatList->rAddUser(who);
                if (msg[0] != '\0') //Message from a new user
                { 
                    char *nameAndMsg;
                    if (hasAlias)
                    {
                        nameAndMsg = new char[strlen(alias)+strlen(msg)+10];
                        sprintf(nameAndMsg, "%c%s: %s", COLOR_RED, alias, msg);
                    }
                    else
                    {
                        nameAndMsg = new char[strlen(userNoAlias)+strlen(msg)+10];
                        sprintf(nameAndMsg, "%c%s: %s", COLOR_RED, userNoAlias, msg);
                    }
                    convos[i]->addText(nameAndMsg);
                    delete [] nameAndMsg;
                }
                chatList->rSetColor(0, who);
                totalConversations++;
                rSetActive(who);
		update = true; 
                return 1; //added new convo
            }
        }
        return -1; //convo list is full
    }
    else
        return 0;
}

void rConversationWindow::rKillConversation(char *name)
{
    for (int i=0; i<totalConversations; i++)
    {
        if (strcasecmp(name, convos[i]->getID()) == 0)
        {
            if (chatRoomMode[i] && *client != NULL && chatRoomIDs[i] >= 0)
                (*client)->rLeaveChatRoom(chatRoomIDs[i]);
            chatList->rKillUser(name);
            delete convos[i];
            chatRoomMode[i] = false;
            chatRoomIDs[i] = -1;
            chatRoomNames[i][0] = '\0';
            update = true;
            for (int j=i; j<maxConversations-1; j++)
            {
                convos[j] = convos[j+1];
                convos[j+1] = NULL;
                chatRoomMode[j] = chatRoomMode[j+1];
                chatRoomIDs[j] = chatRoomIDs[j+1];
                strcpy(chatRoomNames[j], chatRoomNames[j+1]);
                chatRoomMode[j+1] = false;
                chatRoomIDs[j+1] = -1;
                chatRoomNames[j+1][0] = '\0';
            }
            int k;
            currentConversation = -1;
            totalConversations--;
            for (k=i; k<maxConversations; k++)
            {
                 if (convos[k] != NULL)
                     currentConversation = k;
            }
            if (currentConversation == -1)
            {
                for (k=i; k>=0; k--)
                {
                    if (convos[k] != NULL)
                        currentConversation = k;
                }
            }
        }
    }
    rSetActive(chatList->rGetCurrentUser());
}

void rConversationWindow::rKillAllConversations()
{
    for (int i=0; i<maxConversations; i++)
    {
        if(convos[i] != NULL)
        {
            delete convos[i];
            convos[i] = NULL;
        }
        chatRoomMode[i] = false;
        chatRoomIDs[i] = -1;
        chatRoomNames[i][0] = '\0';
    }
    update = true;
    currentConversation = -1;
    totalConversations = 0;
}

bool rConversationWindow::rScrollUp(char *name, int d)
{
    for (int i=0; i<totalConversations; i++)
    {
        if (strcasecmp(name, convos[i]->getID()) == 0)
        {
	    update = true;
            return convos[i]->scrollUp(d);
        }
    }
    return false;
}

bool rConversationWindow::rScrollDown(char *name, int d)
{
    for (int i=0; i<totalConversations; i++)
    {
        if (strcasecmp(name, convos[i]->getID()) == 0)
        {
	    update = true;
            return convos[i]->scrollDown(d);
        }
    }
    return false;
}

void rConversationWindow::rSetActive(char *name)
{

    if (currentConversation > -1) //this is not the first convo
    {
        char *txt = input->rFormat();
        convos[currentConversation]->setUserInput(txt, input->rGetPosition(), input->rGetPosResult());
        delete []txt;
    }

    for (int i=0; i<totalConversations; i++)
    {
        if (strcasecmp(name, convos[i]->getID()) == 0)
        {
            currentConversation = i;
            if (convos[i]->getUserInput() != NULL)
            {
                int pos= 0, res=0;
                input->rSetBuffer(convos[i]->getUserInput(pos,res));
                input->rSetPosition(pos);
                input->rSetPosResult(res);
            }
            else
                input->rClear();
        
        }
    }
}

int rConversationWindow::rNeedsDrawn()
{
    int res = 0;
    res |= input->rNeedsDrawn();
    res |= chatList->rNeedsDrawn();
    res |= buddylist->rNeedsDrawn();
    if (update)
	res |= 2;
    return res;
}
     
int rConversationWindow::rDraw(int res)
{
    int i;
    if (res == SCREEN_TOP) //draw all top screen stuff
    {
	input->rDraw();
	if ( (currentConversation > -1) &&
           (totalConversations > 0) &&
           (convos[currentConversation] != NULL) &&
           (activeText != NULL) &&
           (font != NULL) )
	{
            int xpos = 17;
            int ypos = 28;
	    setFont((uint16**)convos[currentConversation]->getFont());
	    setColor(RGB15(0,0,0));
	    convos[currentConversation]->getActiveText(activeText);
            bool color = false;
            for (i=0; i<(int)strlen(activeText); i++)
            {
                if ((int)activeText[i] == COLOR_RED)
                {
                    setColor(RGB15(31,0,0));
                    color = true;
                }
                else if ((int)activeText[i] == COLOR_BLUE)
                {
                    setColor(RGB15(0,0,31));
                    color = true;
                }
                if ((int)activeText[i] >= 32)
                {
                    if ( ((int)activeText[i] == ':') && color )
                    {
                        setColor(RGB15(0,0,0));
                        color = false;
                    }
                    fb_dispChar(xpos, ypos, activeText[i]);
                    if (color)
                    {
                        xpos++;
                        fb_dispChar(xpos, ypos, activeText[i]);
                    }
                    xpos+=font[activeText[i]-32][0]+1;
                }
                if ((int)activeText[i] == '\n')
                {
                    ypos+=font[0][1]+1;
                    xpos = 17;
                }
            }
	}
        if ( (*client)->rIsAway() )
        {
            setColor(RGB15(31,31,0));
            fb_dispString(110, 1, MiscText[0]);
            fb_dispString(111, 1, MiscText[0]);
            setColor(RGB15(0,0,0));
        }
    }
    else if (res == SCREEN_BOTTOM) //draw all bottom screen stuff
    {
	chatList->rDraw();
	if (buddylist->rIsShown())
        {
	    buddylist->rDraw();
	}
    }
    return (res);
}
   
bool rConversationWindow::rCurrentConvoChanged()
{
    if (currentConversation == -1)
        return false;
    if (totalConversations == 0)
        return false;
    if (convos[currentConversation] == NULL)
        return false;
    return convos[currentConversation]->hasChanged();
}

int rConversationWindow::rParseMainKey(int key, bool *pickup)
{
    int res = 0;
    char userNoAlias[100];
    char alias[100];
    if (input == NULL)
        return 0;
    if (pickup)
        *pickup = true;
    if (key == (unsigned char)rKey::KEY_TRIGGER_R)
    {
        buddylist->rToggleShow();
        return key;
    }
    if (key == (unsigned char)rKey::KEY_BUTTON_START)
    {
    	rKillConversation(chatList->rGetCurrentUser());
	return key;
    }
    if (key == (unsigned char)rKey::KEY_BUTTON_SELECT)
    {
        if (activeMenu)
            delete activeMenu;
        activeMenu = new rMenu(OptionsMenuText[0], OptionsMenuText[1], 10, 10, MENU_MAIN_OPTIONS, (uint16**)convoInstance->font);
        activeMenu->rAddOption(OptionsMenuText[2], 0);
        activeMenu->rAddOption(OptionsMenuText[3], 1);
        activeMenu->rAddOption(OptionsMenuText[4], 2);
        activeMenu->rAddOption(OptionsMenuText[5], 3);
        activeMenu->rAddOption(OptionsMenuText[6], 4);
        activeMenu->rAddOption(OptionsMenuText[7], D_CANCEL);
        return key;
    }
    if (totalConversations == 0)
        return 0;
    if (key == rKey::KEY_NONE)
        return 0;
    res = (unsigned char)input->rPushKey(key);
    char *w = chatList->rGetCurrentUser();
    switch ((unsigned char) res)
    {
        case (unsigned char)rKey::KEY_CHATLIST_SCROLLUP:
	    chatList->rScrollUp(1);
            break;
        case (unsigned char)rKey::KEY_CHATLIST_SCROLLDOWN:
	    chatList->rScrollDown(1);
            break;
        case (unsigned char)rKey::KEY_TEXT_SCROLLUP:
            rScrollUp(w, 1);
            if (pickup)
                *pickup = false;
            break;
        case (unsigned char)rKey::KEY_TEXT_SCROLLDOWN:
            rScrollDown(w, 1);
            if (pickup)
                *pickup = false;
            break;
        case (unsigned char)rKey::KEY_LIST_1ST:
            chatList->rSelectUser(0);
            rSetActive(chatList->rGetCurrentUser());
            break;
        case (unsigned char)rKey::KEY_LIST_2ND:
            chatList->rSelectUser(1);
            rSetActive(chatList->rGetCurrentUser());
            break;
        case (unsigned char)rKey::KEY_LIST_3RD:
            chatList->rSelectUser(2);
            rSetActive(chatList->rGetCurrentUser());
            break;
        case (unsigned char)rKey::KEY_SEND:
            rAddMessage(w);
            input->rClear();
            return true;
        case (unsigned char)rKey::KEY_WARN:
            if (w != NULL && !rIsChatRoomConversation(currentConversation))
            {
                rUtilities::rParseUsername(w, userNoAlias, alias);
                (*client)->rWarn(userNoAlias, false);
            }
            break;
        case (unsigned char)rKey::KEY_BLOCK:
            if (w != NULL && !rIsChatRoomConversation(currentConversation))
            {
                rUtilities::rParseUsername(w, userNoAlias, alias);
                (*client)->rBlock(userNoAlias);
            }
            break;
        default: break;
    }
    return res;
}

int rConversationWindow::rParseBuddyKey(int key, int x, int y, bool *pickup)
{
    int res = 0;
    char *name;
    if ((unsigned char)key == (unsigned char)rKey::KEY_TRIGGER_R)
    {
        buddylist->rToggleShow(); 
        return rKey::KEY_TRIGGER_R;
    }
    else if ((unsigned char)key == (unsigned char)rKey::KEY_DIRECTION_DOWN)
    {
        buddylist->rSelectUser(buddylist->rGetCurrentSelectPosition()+1);
    } 
    else if ((unsigned char)key == (unsigned char)rKey::KEY_DIRECTION_UP)
    {
       buddylist->rSelectUser(buddylist->rGetCurrentSelectPosition()-1);
    } 
    if (key == rKey::KEY_NONE)
        return false;
    res = buddylist->rPushKey(x,y);
    if (pickup)
        *pickup = false;
    if (res == rKey::KEY_BUDDY_IM) //IM button pushed
    {
        name = buddylist->rGetCurrentBuddy();
        if (name != NULL)
        {
            buddylist->rToggleShow(); 
            rAddMessage(name, "\0"); //add an empty message
        }
    }
    else if (res == rKey::KEY_BUDDY_INFO) //Info for buddy requested
    {
        char alias[100];
        char userNoAlias[100];
        name = buddylist->rGetCurrentBuddy();
        if (name != NULL)
        {
            rUtilities::rParseUsername(name, userNoAlias, alias);
            buddylist->rToggleShow(); 
            (*client)->rGetInfo(rUtilities::rNormalize(userNoAlias));
        }
    }
    else if (res == rKey::KEY_BUDDY_ADD) //Add buddy requested
    {
        char alias[100];
        char userNoAlias[100];
        name = NULL;
        if (!rIsChatRoomConversation(currentConversation))
            name = chatList->rGetCurrentUser();
        if (name == NULL)
            name = buddylist->rGetCurrentBuddy();
        if (name != NULL)
        {
            rUtilities::rParseUsername(name, userNoAlias, alias);
            (*client)->rAddBuddy(userNoAlias);
        }
    }
    return res;
}

void rConversationWindow::rSetFont(unsigned short int **fnt) 
{ 
    if (input)
        input->rSetFont(fnt);
    if (chatList)
        chatList->rSetFont(fnt);
    if (buddylist)
        buddylist->rSetFont(fnt);
    font = fnt; 
}

int rConversationWindow::rCurrentScreen()
{
    int current = MAIN_SCREEN;
    if (buddylist->rIsShown())
	current = BUDDY_SCREEN;
    return current;
}

void rConversationWindow::rSetLoginName(char *name)
{
    strcpy(username, name);
}

void rConversationWindow::rSetLoginPassword(char *pass)
{
    strcpy(password, pass);
}

void rConversationWindow::rParseHTML(char *data)
{
    int len = strlen(data);
    char *pch;
    char *nData;
    int i, stop, insLen;
    for (i=0; i<len; i++)
    {
        if (data[i] == '<')
        {
            stop = i;
            while (data[i] != '>')
                i++;
            i++;
            nData = &data[i];
            pch = &data[stop];
            insLen = 0; 
            if (!strncasecmp(pch, "<BR", 3)) //found newline
            {
                data[stop] = '\0';
                strcat(data, "\n");
                insLen = 0;
            }
            else
                data[stop] = '\0';
            strcat(data, nData);
            i = stop+insLen-1;
            len = strlen(data);               
        }
        else if (data[i] == '&')
        {
            int replaced = 0;
            pch = &data[i];
            stop = i;
            if (!strncasecmp(pch, "&lt;", 4))
            {
                nData = &pch[4];
                replaced = 1;
                insLen = 0;
            }
            else if (!strncasecmp(pch, "&gt;", 4))
            {
                nData = &pch[4];
                replaced = 2;
                insLen = 0;
            }
            else if (!strncasecmp(pch, "&quot;", 6))
            {
                nData = &pch[6];
                replaced = 3;
                insLen = 0;
            }
            else if (!strncasecmp(pch, "&amp;", 5))
            {
                nData = &pch[5];
                replaced = 4;
                insLen = 0;
            }
            switch (replaced)
            {
                case 1: data[stop] = '\0'; strcat(data, "<"); strcat(data, nData); i = stop+insLen; len = strlen(data); break;
                case 2: data[stop] = '\0'; strcat(data, ">"); strcat(data, nData); i = stop+insLen; len = strlen(data); break;
                case 3: data[stop] = '\0'; strcat(data, "\""); strcat(data, nData); i = stop+insLen; len = strlen(data); break;
                case 4: data[stop] = '\0'; strcat(data, "&"); strcat(data, nData); i = stop+insLen; len = strlen(data); break;
                default: break;
            }
        }
    }
}
void rConversationWindow::rOnBuddyUpdate(const char* theGroup, const int gID, const char* theBuddy, const char* theAlias, const int theStat)
{
    char sBuddy[100];
    char sAlias[100];
    char sGroup[100];
    char nameAndAlias[200];
    int  prevStat;
    bool isTalking = false;
    strcpy(sBuddy, theBuddy);
    if (theGroup != NULL)
        strcpy(sGroup, theGroup);
    else
        strcpy(sGroup, "Unlisted");
    char id = (char)gID;
    if (theAlias != NULL)
    {
        strcpy(sAlias, theAlias);
        sprintf(nameAndAlias, "%s:%s", theBuddy, theAlias);
    }
    else
        sprintf(nameAndAlias, "%s", theBuddy);

    prevStat = convoInstance->rGetBuddyList()->rGetStats(nameAndAlias);
    isTalking = convoInstance->rGetChatList()->rIsTalking(nameAndAlias);
        
    //WPRINT("UPDATE: %s[%d]", theBuddy, theStat);

    if ( (theStat & BUDDY_AVAILABLE) == BUDDY_AVAILABLE)
    {
        if (isTalking)
        { 
            if ( ( (prevStat & BUDDYVIEW_BUDDY_OFFLINE) == BUDDYVIEW_BUDDY_OFFLINE) ) //user has signed on
                convoInstance->rAddMessage(nameAndAlias, "User signed on", false);
            if ( ((theStat & BUDDY_AWAY) == BUDDY_AWAY) && ( (prevStat & BUDDYVIEW_BUDDY_AWAY) == 0) ) //buddy is now away
                convoInstance->rAddMessage(nameAndAlias, "User has gone away", false);
            if ( ((theStat & BUDDY_IDLE) == BUDDY_IDLE) && ( (prevStat & BUDDYVIEW_BUDDY_IDLE) == 0) ) //buddy is now idle
                convoInstance->rAddMessage(nameAndAlias, "User has gone idle", false);
            if ( ((theStat & BUDDY_AWAY) == 0) && ( (prevStat & BUDDYVIEW_BUDDY_AWAY) == BUDDYVIEW_BUDDY_AWAY) ) //buddy has returned
                convoInstance->rAddMessage(nameAndAlias, "User has returned", false);
            if ( ((theStat & BUDDY_IDLE) == 0) && ( (prevStat & BUDDYVIEW_BUDDY_IDLE) == BUDDYVIEW_BUDDY_IDLE) ) //buddy is active
                convoInstance->rAddMessage(nameAndAlias, "User has become active", false);
        }
            
        convoInstance->rGetBuddyList()->rAddGroup(sGroup, id);
        if (theAlias != NULL)
            convoInstance->rGetBuddyList()->rAddBuddy(sBuddy, sGroup, sAlias);
        else
            convoInstance->rGetBuddyList()->rAddBuddy(sBuddy, sGroup);
    }
    else if ( theStat == BUDDY_UNAVAILABLE) //buddy has signed off
    {
        if (isTalking)
        {
            if ( ( (prevStat & BUDDYVIEW_BUDDY_ONLINE) == BUDDYVIEW_BUDDY_ONLINE) ) //user has signed off
                convoInstance->rAddMessage(nameAndAlias, "User signed off", false);
        }
        convoInstance->rGetBuddyList()->rDeleteBuddy(nameAndAlias); 
        return;
    }
    convoInstance->rGetBuddyList()->rSetAway(nameAndAlias, ( (theStat & BUDDY_AWAY) == BUDDY_AWAY));
    convoInstance->rGetBuddyList()->rSetIdle(nameAndAlias, ( (theStat & BUDDY_IDLE) == BUDDY_IDLE)); 
}

void rConversationWindow::rOnReceiveIM(const char* who, const char* msg, const bool away)
{
    char sBuddy[200];
    char *sMsg;
    sMsg = new char[strlen(msg) + 1];
    strcpy(sMsg, msg);
    strcpy(sBuddy, who);
    convoInstance->rParseHTML(sMsg);
    convoInstance->rAddMessage(sBuddy, sMsg, away);
    delete []sMsg;
}

void rConversationWindow::rOnChatJoin(const char* roomName, const int roomID)
{
    char sRoom[128];
    strncpy(sRoom, roomName, sizeof(sRoom) - 1);
    sRoom[sizeof(sRoom) - 1] = '\0';
    convoInstance->rAddChatRoom(sRoom, roomID);
}

void rConversationWindow::rOnReceiveChat(const char* roomName, const char* who, const char* msg)
{
    char sRoom[128];
    char sWho[200];
    strncpy(sRoom, roomName, sizeof(sRoom) - 1);
    sRoom[sizeof(sRoom) - 1] = '\0';
    strncpy(sWho, who, sizeof(sWho) - 1);
    sWho[sizeof(sWho) - 1] = '\0';
    convoInstance->rAddChatRoomMessage(sRoom, sWho, (char*)msg);
}

void rConversationWindow::rOnGetInfo(const char* info)
{
    char *sMsg;
    sMsg = new char[strlen(info) + 1];
    strcpy(sMsg, info);
    convoInstance->rParseHTML(sMsg);
    convoInstance->rDisplayProfile(sMsg);
    delete []sMsg;
}

void rConversationWindow::rOnNick(const char *name)
{
    char sName[100];
    strcpy(sName, name);
    convoInstance->rSetLoginName(sName);
}

void rConversationWindow::rOnError(const int id, const char *desc)
{
    char *msg;
    if (id == SERVER_ERROR_DISCONNECT)
    {
        msg = new char[100];
        sprintf(msg, "%s\nPlease log in again.", desc);
        if (activeDialog)
            delete activeDialog;
        activeDialog = new rDialog(msg, OKCancelText[0], NULL, 10, 10, DIALOG_DISCONNECT, (uint16**)convoInstance->font);
        convoInstance->rSetStatus(STATUS_DISCONNECT);
        delete []msg;
    }
    else //general error
    {
        msg = new char[strlen(desc)+1];
        strcpy(msg, desc);
        if (activeDialog)
        {
            delete activeDialog;
        }
        activeDialog = new rDialog(msg, OKCancelText[0], NULL, 10, 10, DIALOG_GENERALERROR, (uint16**)convoInstance->font);
        delete []msg;
    }
}

    
void rConversationWindow::rMenuDone(const int retValue, const int menu)
{
    switch (menu)
    {
        case MENU_MAIN_OPTIONS:
            switch (retValue)
            {
                case 0: //User selected to send an IM!
                    rDoSendIM();
                    break;
                case 1: //User selected to join a chatroom!
                    rDoJoinChatRoom();
                    break;
                case 2: //User selected to get user info!
                    rDoGetInfo();
                    break;
                case 3: //User selected to set away message!
                    rDoAwayMessage();
                    break;
                case 4: //User selected to sign off!
                    rDoSignOff(true);
                    break;
                default: break;
            }
            break;
        default: break;
    }
}

void rConversationWindow::rDialogDone(const int retValue, const int dialog)
{
    WPRINT("dialog is %d %d", dialog, retValue);
    switch (dialog)
    {
        case DIALOG_DISCONNECT:
            chatList->rKillAllUsers();
            buddylist->rDeleteAllBuddies();
            chatList->rSetFont(font);
            buddylist->rSetFont(font);
            input->rClear();
            rKillAllConversations();
            break;
        case DIALOG_SIGNOFF:
            if (retValue == D_OK)
                rDoSignOff(false);
        default: break;
    }

/*    if ( (dialog == MENU_CONNECT_TYPE) && (retValue == CONNECT_TYPE_DHCP) )
    {
        tmp = new rMenu("DHCP Setup", "Please choise an access point.", 10, 20, MENU_ACCESS_POINTS, font);
        tmp->rAddOption("linksys [WEP] 100%", 1);
        tmp->rAddOption("netgear [WEP] 26%", 2);
        tmp->rAddOption("dlink 20%", 3);
        tmp->rAddOption("gigabit 80%", 4);
        tmp->rAddOption("GATEWAY [WEP] 40%", 5);
        *activeMenu = tmp;
    }*/
}

void rConversationWindow::rListBoxDone(const int retValue, const int dialog)
{
/*    switch (dialog)
    {
        case DIALOG_DISCONNECT:
            chatList->rKillAllUsers();
            buddylist->rDeleteAllBuddies();
            chatList->rSetFont(font);
            buddylist->rSetFont(font);
            input->rClear();
            rKillAllConversations();
            break;
        default: break;
    }

    if ( (dialog == MENU_CONNECT_TYPE) && (retValue == CONNECT_TYPE_DHCP) )
    {
        tmp = new rMenu("DHCP Setup", "Please choise an access point.", 10, 20, MENU_ACCESS_POINTS, font);
        tmp->rAddOption("linksys [WEP] 100%", 1);
        tmp->rAddOption("netgear [WEP] 26%", 2);
        tmp->rAddOption("dlink 20%", 3);
        tmp->rAddOption("gigabit 80%", 4);
        tmp->rAddOption("GATEWAY [WEP] 40%", 5);
        *activeMenu = tmp;
    }*/
}

void rConversationWindow::rEditBoxDone(const int retValue, const int dialog, const char *txt)
{
    char name[200];
    switch (dialog)
    {
        case EDITBOX_SENDIM:
            strcpy(name, txt);
            if (retValue == D_OK)
            {
                char *theAlias = buddylist->rGetBuddyAlias(name);
                rAddMessage(theAlias, "\0"); //add an empty message
            }
        break;
        case EDITBOX_JOINCHAT:
            strcpy(name, txt);
            if (retValue == D_OK)
            {
                bool hasVisibleText = false;
                for (int i = 0; name[i] != '\0'; i++)
                {
                    if (name[i] != ' ' && name[i] != '\t' && name[i] != '\r' && name[i] != '\n')
                    {
                        hasVisibleText = true;
                        break;
                    }
                }
                if (hasVisibleText)
                    (*client)->rJoinChatRoom(name);
            }
        break;
        case EDITBOX_GETINFO:
            strcpy(name, txt);
            if (retValue == D_OK)
            {
                (*client)->rGetInfo(name);
            }
        break;
        default: break;
    }
}

void rConversationWindow::rMultiEditBoxDone(const int retValue, const int dialog, const char *txt)
{
    switch (dialog)
    {
        case EDITBOX_AWAY:
            if (retValue == D_OK)
            {
                bool hasVisibleText = false;
                for (int i = 0; txt[i] != '\0'; i++)
                {
                    if (txt[i] != ' ' && txt[i] != '\t' && txt[i] != '\r' && txt[i] != '\n')
                    {
                        hasVisibleText = true;
                        break;
                    }
                }
                if (hasVisibleText)
                    (*client)->rSetAwayMessage(txt);
                else
                    (*client)->rSetAway(NULL);
            }
        break;
        default: break;
    }
}

void rConversationWindow::rDoSignOff(bool ask)
{
    if (ask)
    {
        if (activeDialog)
            delete activeDialog;
        activeDialog = new rDialog(DialogText[DIALOG_TEXT_SIGNOFF], OKCancelText[2], OKCancelText[3], 10, 10, DIALOG_SIGNOFF, (uint16**)convoInstance->font);
    }
    else
    {
        chatList->rKillAllUsers();
        buddylist->rDeleteAllBuddies();
        chatList->rSetFont(font);
        buddylist->rSetFont(font);
        input->rClear();
        rKillAllConversations();
    }
}

void rConversationWindow::rDoAwayMessage()
{
    if (activeMultiEditBox)
        delete activeMultiEditBox;
    activeMultiEditBox = new rMultiEditBox(DialogText[DIALOG_TEXT_AWAYMESSAGE], OKCancelText[0], OKCancelText[1], 10, 10, 120, 2, EDITBOX_AWAY, (uint16**)convoInstance->font);
}

void rConversationWindow::rDoSendIM()
{
    if (activeEditBox)
        delete activeEditBox;
    activeEditBox = new rEditBox(DialogText[DIALOG_TEXT_SENDIM], OKCancelText[0], OKCancelText[1], 10, 10, 100, EDITBOX_SENDIM, (uint16**)convoInstance->font);
}

void rConversationWindow::rDoJoinChatRoom()
{
    if (activeEditBox)
        delete activeEditBox;
    activeEditBox = new rEditBox(DialogText[DIALOG_TEXT_JOINCHAT], OKCancelText[0], OKCancelText[1], 10, 10, 100, EDITBOX_JOINCHAT, (uint16**)convoInstance->font);
}

void rConversationWindow::rDoGetInfo()
{
    if (activeEditBox)
        delete activeEditBox;
    activeEditBox = new rEditBox(DialogText[DIALOG_TEXT_GETPROFILE], OKCancelText[0], OKCancelText[1], 10, 10, 100, EDITBOX_GETINFO, (uint16**)convoInstance->font);   
}

void rConversationWindow::rDisplayProfile(char *data)
{
    if (activeListBox)
        delete activeListBox;
    activeListBox = new rListBox(191, 10, LISTBOX_GETINFO, (uint16**)convoInstance->font);
    (activeListBox)->rAddString(data);
}

void rConversationWindow::debugMenu()
{
        activeMenu = new rMenu(OptionsMenuText[0], OptionsMenuText[1], 10, 50, MENU_MAIN_OPTIONS, (uint16**)font);
        (activeMenu)->rAddOption(OptionsMenuText[2], 0);
        (activeMenu)->rAddOption(OptionsMenuText[3], 1);

        (activeMenu)->rAddOption(OptionsMenuText[4], 2);

        (activeMenu)->rAddOption(OptionsMenuText[6], 3);
        (activeMenu)->rAddOption(OptionsMenuText[7], 4);
}
