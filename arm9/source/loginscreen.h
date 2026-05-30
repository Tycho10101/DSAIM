#ifndef _LOGINSCREEN_
#define _LOGINSCREEN_
#include "stdio.h"
#include "keyboard.h"
#include "textdisplay.h"
#include "input.h"
#include <../lib/rTOC2/toc2.h>

class rLoginScreen
{
public:
    rLoginScreen(rTOC2 **);
    ~rLoginScreen();
    void rSetFont(unsigned short int **fnt) { font = fnt; usernameInput->rSetFont(font); passwordInput->rSetFont(font); update = true;}
    unsigned short int** rGetFont() { return font; }
    char *rGetUsername() { return usernameInput->rGetUnformattedBuffer(); }
    char *rGetPassword() { return passwordInput->rGetUnformattedBuffer(); }
    bool rIsShown() { return isActive; }
    void rSetStatusString(char*);
    
    int rDraw();
    int rExecute(rKeyboard *kb);
    int rParseKey(int);
    int rNeedsDrawn();
    static void rStatusHandler(char *);
    static void rOnError(const int, const char*);
    static void rOnSignOn();
    static void rOnStatus(const int, const char *);
    static void rOnDebug(const int, const char *);

//protected:
public:
    void drawAll();
    bool isError;
    int errorNum;

private:
    rLoginScreen();

    rKeyboard *keyboard;
    rUserInput *usernameInput;
    rUserInput *passwordInput;
    rTOC2 **client;
    char status[100];
    unsigned short int statX, statY;

    int activeInput;
    bool isActive;
    unsigned short int **font;
    int loginState;
    bool update;
};

#endif
     
