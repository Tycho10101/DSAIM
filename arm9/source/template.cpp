#include "nds.h"
#include <nds/arm9/console.h> //basic print funcionality
#include "globalDefs.h"
#include <dswifi9.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "wificonnect.h"
#ifdef WIFIDEBUG
#include "../lib/rTOC2/wifidebug.h"
#endif
#include <../lib/libfb/libcommon.h>
#include "top_bin.h"
#include "bottom_bin.h"
#include "gba-jpeg-decode.h"
#include "convo.h"
#include "keyboard.h"
#include "input.h"
#include "chatlist.h"
#include "clock.h"
#include "arial8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "textdisplay.h"
#include "loginscreen.h"
#include "commonObjs.h"
#include "menu.h"
#include "menudefs.h"
#include "dialog.h"
#include "listbox.h"
#include "editbox.h"
#include "multieditbox.h"
#include "wificonnect.h"
#define sleep(x) for(int z_x_p=0; z_x_p < x; z_x_p++) swiWaitForVBlank()
#ifdef WIFIDEBUG
#define WPRINT(format, args...)  rWifiDebug::rPrint("Template: %s :" format,__func__, ##args)
#else
#define WPRINT(format, args...)
#endif

rConversationWindow *chatwnd;
rLoginScreen        *loginScreen;
rKeyboard           *keyboard;
rMenu               *activeMenu;
rDialog             *activeDialog;
rListBox            *activeListBox;
rEditBox            *activeEditBox;
rMultiEditBox       *activeMultiEditBox;
rClock              *clock = NULL;
rTOC2               *client;
bool	            globalRedraw = false;
bool                lidClosed;
bool                debugOn = false;
static char         emptyAwayMessage[] = "";

touchPosition touchXY;
int currentState = STATE_LOGIN;
int keyHit = 0;
unsigned short x = 0, y = 0;

void DrawScreens();
void CheckForDebug();

void OnDebug(const int num, const char *error)
{
    char err[500];
    sprintf(err, "DEBUG: %s", error);
    
}

void OnError(const int num, const char *error)
{
    iprintf("ERROR: %s\n", error);
    while (1) {}
}
	
//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------
        
    char activeText[650];
    SERVER_MESSAGE_TYPES currentMsg;
    bool needPickup = false;

#ifdef WIFIDEBUG
    rWifiDebug::rInit();
#endif
    
    irqSet(IRQ_VBLANK, 0);
    irqEnable(IRQ_VBLANK);
    vramSetBankC(VRAM_C_SUB_BG);
    irqSet(IRQ_VBLANK, 0);
    irqEnable(IRQ_VBLANK);
    fb_init();
    bg_init();
#ifdef WIFIDEBUG
    CheckForDebug();
#endif
    irqSet(IRQ_VBLANK, 0);
    irqEnable(IRQ_VBLANK);
    u16 *img = new u16[256*256];
    chatwnd = new rConversationWindow();
    memset(activeText, 0, sizeof(activeText));
    chatwnd->rSetOutputBuffer(activeText);
    keyboard = new rKeyboard();
    clock = new rClock();
    client = new rTOC2();
	initWifi();
    chatwnd->rSetTOC(&client);
    chatwnd->rSetFont((uint16**)font_arial_8);
    chatwnd->rSetStatus(STATUS_NOTOC);
    keyboard->rSetFont((uint16**)font_arial_8);
    clock->rSetFont((uint16**)font_arial_8);
    loginScreen = new rLoginScreen(&client);
    loginScreen->rSetFont((uint16**)font_arial_8);
    loginScreen->rSetStatusString(LoginText[8]);
    //WPRINT("decompressing background");
    JPEG_DecompressImage(top_bin, (u16*)img, 256, 192);
    //WPRINT("setting background");
    fb_setBG((uint16*)img);

    //WPRINT("decompressing foreground");
    JPEG_DecompressImage(bottom_bin, (u16*)img, 256, 192);
    //WPRINT("setting foreground");
    bg_setBG((uint16*)img);
    activeMenu = NULL;
    activeDialog = NULL;
    activeListBox = NULL;
    activeEditBox = NULL;
    activeMultiEditBox = NULL;
//    chatwnd->debugMenu();
    lidClosed = false;
    globalRedraw = true;

    while(pmMainLoop()) 
    {
        scanKeys();

        /* CLIENT LISTENS TO AIM */
        if (chatwnd->rGetStatus() == STATUS_NORMAL)
	{
            client->rListen(currentMsg);
	    //check for lid closing to set away message
	    if ((keysHeld() & KEY_LID) == KEY_LID) //lid is closed
	    {
                if (!lidClosed)
                {
		    WPRINT("lid is closed!");
                    client->rSetSpecialAway(MiscText[1]);
                    lidClosed = true;
                }
            }
            else if (lidClosed)
            {
		WPRINT("lid is opened");
                client->rSetSpecialAway(emptyAwayMessage);
                lidClosed = false;
		globalRedraw = true;
            }    
	}
        
        /* CHECK TO SEE IF A DIALOG IS SHOWING */
        if (activeDialog)
        {
            int ret, dialog;
            ret = activeDialog->rExecute(keyboard);
            if (ret != D_CONTINUE)
            {
                dialog = activeDialog->rGetDialogID();
                chatwnd->rDialogDone(ret, dialog);
                if (rWifiConnect::rCheckNetSetupStep(ret, font_arial_8) == -1)
                {
                    loginScreen = new rLoginScreen(&client);
                    loginScreen->rSetFont((uint16**)font_arial_8);
		    chatwnd->rSetStatus(STATUS_NOTOC); 
                }
                switch (dialog)
                {
                    case DIALOG_SIGNOFF:
                        if (ret != D_OK)
                            break;
                        client->rDisconnect();
                        chatwnd->rSetStatus(STATUS_DISCONNECT);
                    case DIALOG_DISCONNECT: //IF WE ARE DISCONNECTED!
                        delete client;
                        client = new rTOC2();
                        chatwnd->rSetTOC(&client);
                        loginScreen = new rLoginScreen(&client);
                        loginScreen->rSetFont((uint16**)font_arial_8);
		        chatwnd->rSetStatus(STATUS_NOTOC);
                        break;
                    default: break;
                }
                if (activeDialog->rHasRun())
                { 
                    delete activeDialog;
                    activeDialog = NULL;
                }
                globalRedraw = true;
           }
        }
        /* CHECK TO SEE IF A EDITBOX IS SHOWING */
        else if (activeEditBox)
        {
            int ret, dialog;
            char editText[200];
            ret = activeEditBox->rExecute(keyboard);
            if (ret != D_CONTINUE)
            {
                dialog = activeEditBox->rGetDialogID();
                strcpy(editText, activeEditBox->rGetText());
                chatwnd->rEditBoxDone(ret, dialog, editText);
                rWifiConnect::rCheckNetSetupStep(ret, font_arial_8, (void*)editText);
                if (activeEditBox->rHasRun())
                {
                    delete activeEditBox;
                    activeEditBox = NULL;
                }
                globalRedraw = true;
           }
        }   
        /* CHECK TO SEE IF A MULTI EDITBOX IS SHOWING */
        else if (activeMultiEditBox)
        {
            int ret, dialog;
            char editText[1024];
            ret = activeMultiEditBox->rExecute(keyboard);
            if (ret != D_CONTINUE)
            {
                dialog = activeMultiEditBox->rGetDialogID();
		char *resString = activeMultiEditBox->rGetText();
                strcpy(editText, resString);
		delete [] resString; //must be deleted since it is allocated in the GetText()
                chatwnd->rMultiEditBoxDone(ret, dialog, editText);
                rWifiConnect::rCheckNetSetupStep(ret, font_arial_8, (void*)editText);
                if (activeMultiEditBox->rHasRun())
                {
                    delete activeMultiEditBox;
                    activeMultiEditBox = NULL;
                }
                globalRedraw = true;
           }
        }   
        /* CHECK TO SEE IF A LISTBOX IS SHOWING */
        else if (activeListBox)
        {
            int ret, dialog;
            ret = activeListBox->rExecute(keyboard);
            if (ret != D_CONTINUE)
            {
                void *extra;
                dialog = activeListBox->rGetListID();
                if (ret == D_OK)
                    ret = activeListBox->rGetSelection();
                extra = activeListBox->rGetCurrentData();
                chatwnd->rListBoxDone(ret, dialog);
                rWifiConnect::rCheckNetSetupStep(ret, font_arial_8, extra);
                if (activeListBox->rHasRun())
                {
                    delete activeListBox;
                    activeListBox = NULL;
                }
                globalRedraw = true;
           }
        }
        /* CHECK TO SEE IF A MENU IS SHOWING */
        else if (activeMenu)
        {
            int ret, menu;
            ret = activeMenu->rExecute(keyboard);
            if (ret != D_CONTINUE)
            {
                menu = activeMenu->rGetMenuID();
                chatwnd->rMenuDone(ret,menu);
                if (rWifiConnect::rCheckNetSetupStep(ret, font_arial_8) == -1) //user canceled net setup
                {
                    loginScreen = new rLoginScreen(&client);
                    loginScreen->rSetFont((uint16**)font_arial_8);
		    chatwnd->rSetStatus(STATUS_NOTOC); 
                }
                if (activeMenu->rHasRun())
                {
                    delete activeMenu;
                    activeMenu = NULL;
                }
                globalRedraw = true;
            }
        }
        /* CHECK TO SEE IF A LOGIN SCREEN IS SHOWING */
        else if (loginScreen)
        {
            int loginStatus = loginScreen->rExecute(keyboard);
            if (loginStatus == 1)
            {
                client->rSetBuddyUpdateHandler(rConversationWindow::rOnBuddyUpdate);
                client->rSetNickHandler(rConversationWindow::rOnNick);
                client->rSetReceiveIMHandler(rConversationWindow::rOnReceiveIM);
                client->rSetChatJoinHandler(rConversationWindow::rOnChatJoin);
                client->rSetReceiveChatHandler(rConversationWindow::rOnReceiveChat);
                client->rSetErrorHandler(rConversationWindow::rOnError);
                client->rSetGetInfoHandler(rConversationWindow::rOnGetInfo);
                chatwnd->rSetStatus(STATUS_NORMAL);
                client->rSetSignOnHandler(NULL);
                delete loginScreen;
                loginScreen = NULL;
                globalRedraw = true;
            }
	    else if (loginStatus == 2) //time for network setup
	    {
                client->rSetSignOnHandler(NULL);
                delete loginScreen;
                loginScreen = NULL;
                globalRedraw = true;
                rWifiConnect::rDoAPSetupMenu((uint16**)font_arial_8);
            }
            else if (loginStatus == -1) //Cannot connect to wifi (Go to network setup?)
            {
                client->rSetSignOnHandler(NULL);
                delete loginScreen;
                loginScreen = NULL;
                globalRedraw = true;
                rWifiConnect::rDoAPSetupMenu((uint16**)font_arial_8);
            }
        } 
        else //Normal DSAIM PROCESS
        {
            keyHit = keyboard->rCheckKey2(chatwnd->rCurrentScreen(), &x, &y, &needPickup);
            switch (chatwnd->rCurrentScreen())
            {
                case MAIN_SCREEN:
                    chatwnd->rParseMainKey(keyHit, &needPickup);
                    break;
                case BUDDY_SCREEN:
                    chatwnd->rParseBuddyKey(keyHit, x, y, &needPickup);
                    break;
                default: break;
            }
        }
	swiWaitForVBlank();
        DrawScreens();
    }

    return 0;
}


void DrawScreens()
{
    int doRedraw = 0;
    doRedraw |= keyboard->rNeedsDrawn();
    doRedraw |= chatwnd->rNeedsDrawn();
    if (loginScreen)
        doRedraw |= loginScreen->rNeedsDrawn();
    if (activeMenu)
    {
        doRedraw |= activeMenu->rNeedsDrawn();
    }
    if (activeDialog)
    {
        doRedraw |= activeDialog->rNeedsDrawn();
    }
    if (activeListBox)
        doRedraw |= activeListBox->rNeedsDrawn();
    if (activeEditBox)
        doRedraw |= activeEditBox->rNeedsDrawn();
    if (activeMultiEditBox)
        doRedraw |= activeMultiEditBox->rNeedsDrawn();
    if (clock)
        doRedraw |= clock->rNeedsDrawn();

    if (globalRedraw)
    {
        globalRedraw = false;
        WPRINT("redrawing whole screen");
        doRedraw = 0xFFFFFFFF;
    }
    //DRAW PASS
    if (doRedraw & 1)
    {
	keyboard->rDraw();
	chatwnd->rDraw(SCREEN_BOTTOM);
        if (activeMenu)
        {
            activeMenu->rDraw();
        }
        if (activeDialog)
        {
            activeDialog->rDraw();
        }
        if (activeListBox)
        {
            activeListBox->rDraw();
        }
    }
    if (doRedraw & 2)
    {	
        if (!loginScreen)
	    chatwnd->rDraw(SCREEN_TOP);
        if (clock)
            clock->rDraw();
        if (loginScreen)
            loginScreen->rDraw();
        if (activeEditBox)
            activeEditBox->rDraw();
        if (activeMultiEditBox)
            activeMultiEditBox->rDraw();
    }
    //SWAP PASS
    if (doRedraw & 1)
    {
        bg_swapBuffers();
    }
    if (doRedraw & 2)
    {
        fb_swapBuffers();
    }
}

void CheckForDebug()
{
#ifdef WIFIDEBUG
    int pressed = 0;
    scanKeys();
    pressed = keysHeld();
    if ( (pressed & KEY_R) == KEY_R )
    {
        debugOn = true;
        autoConnect();
        rWifiDebug::rConnect("192.168.1.2", 2702);
    }
#endif
}

