#include <string.h>
#include <stdio.h>
#include "nds.h"
#include "menudefs.h"
#include <../lib/libfb/libcommon.h>
#include "input.h"
#include <stdarg.h>
#include "loginscreen.h"
#include "globalDefs.h"
#include "wificonnect.h"
#ifdef WIFIDEBUG
#include "../lib/rTOC2/wifidebug.h"
#endif
#include "debug.h"

#ifdef WIFIDEBUG
#define WPRINT(format, args...)  rWifiDebug::rPrint("Login Screen: %s :" format,__func__, ##args)
#else
#define WPRINT(format, args...)
#endif

rLoginScreen *loginInstance;
rLoginScreen::rLoginScreen() {}

rLoginScreen::rLoginScreen(rTOC2 **toc)
{
    WPRINT("create login screen");
    loginState = 0;
    client = toc;
    usernameInput = new rUserInput(1, 1000);
    passwordInput = new rUserInput(1, 1000);
    activeInput = 0;
    font = NULL;
    status[0] = '\0';
    isActive = true;
    isError = false;
    loginInstance = this;
    update = false;
//    (*client)->rSetDebugHandler(rOnDebug);
}

rLoginScreen::~rLoginScreen()
{
    WPRINT("deleting login screen");
    delete usernameInput;
    delete passwordInput;
}

int rLoginScreen::rParseKey(int key)
{
    int res = 0;          
    if (activeInput == 0)
        res = usernameInput->rPushKeySingleLine(key,189);
    else
        res = passwordInput->rPushKeySingleLine(key,189);
    if (res == rKey::KEY_NAV_DOWN || res == rKey::KEY_NAV_UP)
        activeInput++; activeInput %=2;
    if (res == rKey::KEY_SEND)
    {
        if ( (strlen(usernameInput->rGetUnformattedBuffer()) == 0) || (strlen(passwordInput->rGetUnformattedBuffer()) == 0))
            return rKey::KEY_NONE;
    }
    return key;
}

int rLoginScreen::rDraw()
{
    if (font == NULL)
        return 0;
    setFont(font);
    fb_drawRect(4,24,252, 188, RGB15(0,0,0));
    fb_drawRect(6,26,250, 186, RGB15(31,31,31));

    setColor(RGB15(0,0,0));
    fb_dispString(28, 72, LoginText[0]);
    fb_drawRect(28,86,226, 100, RGB15(0,0,0));
    if (activeInput == 0)
        fb_drawRect(29,87,225, 99, RGB15(27,27,0));
    else
        fb_drawRect(29,87,225, 99, RGB15(26,26,26));
    usernameInput->rDraw(31,87);
    

    fb_dispString(28, 109, LoginText[1]);
    fb_drawRect(28,123,226, 137, RGB15(0,0,0));
    if (activeInput == 1)
        fb_drawRect(29,124,225, 136, RGB15(27,27,0));
    else
        fb_drawRect(29,124,225, 136, RGB15(26,26,26));
    passwordInput->rDraw(31,124);
    setColor(RGB15(0,0,0));
    
        

    fb_dispString(65, 34, LoginText[3]);
    fb_dispString(66, 34, LoginText[3]);

    setColor(RGB15(25,25,0));
    fb_dispString(124, 34, LoginText[4]);
    fb_dispString(125, 34, LoginText[4]);
    setColor(RGB15(0,0,0));
    fb_dispString(70, 50, LoginText[5]);
    fb_dispString(71, 50, LoginText[5]);

    fb_dispString(126, 50, LoginText[6]);
    fb_dispString(127, 50, LoginText[6]);

    if (status[0] != '\0')
    {
        fb_dispString(statX, statY, status);
        fb_dispString(statX+1, statY, status);
    }
   return 2;
}

void rLoginScreen::rSetStatusString(char *msg)
{
    statY = 150;
    strcpy(status, msg);
    int i;
    int len = 0;
    for (i=0; i<(int)strlen(status); i++)
        len += font[status[i]-32][0]+1;
    statX = ((252-4)-(len))/2;
    update = true;
}

int rLoginScreen::rExecute(rKeyboard *kb)
{
    unsigned short x, y;
    int keyHit;
    keyboard = kb;
    switch (loginState)
    {
        case 0:
            update = true;
            keyHit = keyboard->rCheckKey2(LOGIN_SCREEN, &x, &y);//readKey();
            if (keyHit == (unsigned char)rKey::KEY_BUTTON_SELECT) // select was hit;
            {
                loginState = 7;
                break;
            }
            keyHit = rParseKey((unsigned char)keyHit);
            if (strlen(rGetUsername()) > 0 && strlen(rGetPassword()) > 0)
                rSetStatusString(LoginText[7]);
            else
                rSetStatusString(LoginText[8]);
            if (keyHit == (unsigned char)rKey::KEY_SEND)
            {
                loginState = 1;
                rSetStatusString(LoginText[9]);
#ifdef USEWIFI
                rWifiConnect::rInitWifi();
	        irqSet(IRQ_VBLANK, 0);
                irqEnable(IRQ_VBLANK);
#endif           
                update = true;
            }
            break;
        case 1:
#ifdef USEWIFI
            int wifiStatus;
            wifiStatus = rWifiConnect::rConnect();
            update = true;
            switch (wifiStatus)
            {
                case ASSOCSTATUS_SEARCHING:         rSetStatusString(LoginText[14]); break;
                case ASSOCSTATUS_ASSOCIATING:       rSetStatusString(LoginText[15]); break;
                case ASSOCSTATUS_ACQUIRINGDHCP:     rSetStatusString(LoginText[17]); break;
                case ASSOCSTATUS_ASSOCIATED:        rSetStatusString(LoginText[10]); loginState = 2; break;
                case ASSOCSTATUS_CANNOTCONNECT:     rSetStatusString(LoginText[11]); loginState = 3; break;
                default: break;
            }
#else
            loginState = 5;
#endif
            break;
        case 2:
            rSetStatusString(LoginText[18]);
            update = true;
            if (!rWifiConnect::rTestConnection())
            {
                rSetStatusString(LoginText[12]);
                loginState = 6;
                break;
            }
            rSetStatusString(LoginText[10]);
            (*client)->rSetSignOnHandler(rLoginScreen::rOnSignOn);
            (*client)->rSetErrorHandler(rLoginScreen::rOnError);
            loginState = 4;
            break;
        case 3:
            keyboard->rWaitForTouch();
            loginState = 0;
            return -1;
            break;
        case 4:
            int connectStatus;
            (*client)->rApplyDefaultServers();
            connectStatus = (*client)->rConnect(rGetUsername(), rGetPassword());
            if (connectStatus == SERVER_ERROR_UNKNOWN)
            {
                delete *client;
                *client = new rTOC2();
                loginState = 6;
                update = true;    
                rSetStatusString(LoginText[12]);
            }
            else if (connectStatus != ERROR_NO_ERROR)
            {
                delete *client;
                *client = new rTOC2();
                loginState = 6;
                update = true;
                rSetStatusString(LoginText[12]);
            }
            else
            {
                loginState =5;
            }
            break;
        case 5:
            sleep(100);
            return 1;
            break;
        case 6:
            sleep(150);
            loginState = 0;
            break;
        case 7:
            return 2;
            break;
        default: break;
    }
    return 0;
}
            
            
            
void rLoginScreen::rOnError(const int num, const char *error)
{
    char err[100];
    sprintf(err, "ERROR: %s", error);
    loginInstance->rSetStatusString(err);
    loginInstance->errorNum = num;
    loginInstance->isError = true;
}

void rLoginScreen::rOnSignOn()
{
    loginInstance->rSetStatusString(LoginText[13]);
    loginInstance->isError = false;
}

void rLoginScreen::rOnStatus(const int num, const char *error)
{
    char err[100];
    sprintf(err, "- %s -", error);
    loginInstance->rSetStatusString(err);
    loginInstance->errorNum = num;
    loginInstance->isError = false;
    loginInstance->drawAll();
}

void rLoginScreen::rOnDebug(const int num, const char *error)
{
    char err[100];
    sprintf(err, "- %s -", error);
    loginInstance->rSetStatusString(err);
    loginInstance->errorNum = num;
    loginInstance->isError = false;
    loginInstance->drawAll();
    sleep(100);
}

void rLoginScreen::drawAll()
{    
    keyboard->rDraw();
    rDraw();
    fb_swapBuffers();
    bg_swapBuffers();
}

int rLoginScreen::rNeedsDrawn()
{
    if (update)
    {
        update = false;
        return 2;
    }
    else
        return 0;
}
 
void rLoginScreen::rStatusHandler(char *st)
{
    loginInstance->rSetStatusString(st);
}

