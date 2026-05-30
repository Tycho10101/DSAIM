#ifndef _DIALOG_
#define _DIALOG_
#include "stdio.h"
#include "keyboard.h"
#include "textdisplay.h"

class rDialog
{
public:
    rDialog(char *, char *, char *, int, int, int, unsigned short int ** fnt, bool derived = false);
    rDialog();
    virtual ~rDialog();
    void rSetFont(unsigned short int **fnt) { font = fnt;}
    unsigned short int** rGetFont() { return font; }
    bool rIsShown() { return isActive;}
    void rSetPosition(int x, int y) { drawX = x; drawY = y; }
    virtual int rDraw();
    virtual int rExecute(rKeyboard *);
    virtual int rParseKey(int, int, int);
    virtual int rNeedsDrawn();
    int rGetDialogID() { return dialogID; }
    bool rHasRun() { return hasRun; }

private:
    virtual void rCalcPos();
protected:
    rKeyboard *keyboard;
    rTextDisplay *dialogList;
    int dialogID;
    char status[100];
    char *sOK;
    int okPosX;
    int okLen;
    char *sCancel;
    unsigned short int **font;
    int cancelPosX;
    int okPosY;
    int currentSelect;
    int cancelLen;
    bool isActive;
    bool update;
    int drawX, drawY;
    int delayKill;
    int returnValue;
    int finalLen;
    bool hasRun;
};
#endif
