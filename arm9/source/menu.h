#ifndef _MENU_
#define _MENU_
#include "stdio.h"
#include "keyboard.h"
#include "textdisplay.h"
#include "input.h"

class rMenu
{
public:
    rMenu(char *, char *, int, int, int, unsigned short int **);
    ~rMenu();
    void rSetFont(unsigned short int **fnt) { font = fnt;}
    unsigned short int** rGetFont() { return font; }
    bool rIsShown() { return isActive;}
    void rSetPosition(int x, int y) { drawX = x; drawY = y; }
    void rAddOption(char *, int);

    int rDraw();
    int rExecute(rKeyboard *);
    int rParseKey(int, int, int);
    int rNeedsDrawn();
    int rGetMenuID() { return menuID; }
    bool rHasRun() { return hasRun; }

private:
    rMenu();
    void rCalcPos();
    
    rKeyboard *keyboard;
    rTextDisplay *menuList;
    char *title, *desc;
    int menuID;
    char status[100];
    int tx, dx;
    bool isActive;
    unsigned short int **font;
    int currentSelect;
    bool update;
    int drawX, drawY;
    int delayKill;
    int returnValue;
    bool hasRun;
};
#endif
    

    
    
