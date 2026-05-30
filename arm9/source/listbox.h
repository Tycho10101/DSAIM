#include "textdisplay.h"
#include "stdio.h"
#include "keyboard.h"

#ifndef _LISTBOX_
#define _LISTBOX_

#define LISTBOX_DEFAULTHEIGHT 10
#define LISTBOX_DEFAULTWIDTH 85
class rListBox
{
public:
    rListBox(int, int, int, unsigned short int**);
    ~rListBox();

    int rDraw();
    int rNeedsDrawn();
    int rAddString(char *);
    bool rInsertData(int, void *);
    bool rDeleteString(int);

    void rDeleteAll();
    bool rScrollUp(int);
    bool rScrollDown(int);
    char *rGetText(int);
    void *rGetData(int);
    char *rGetCurrentText();
    void *rGetCurrentData();
    void rSetFont(unsigned short**);
    int rParseKey(int, int, int);
    bool rSelectPos(int, bool);
    int rExecute(rKeyboard *);

    void rToggleShow();
    bool rIsShown() { return showList; }
    int rGetListID() { return dialogID; }
    int rGetSelection() { return currentSelect; }
    int rGetCurrentLine() { return list->getCurrentLine(); }
    void rSetCurrentLine(int pos) { list->setCurrentLine(pos); }
    int rGetTotal() { return list->getTotalLines(); }
    void rSetPopulateFunction(void (*func)(void *), int c) { populateFunc = func; totalCycles = c; currentCycle = 0;}
    int  rGetWidth() { return width; }
    int  rGetHeight() { return height; }
    bool rHasRun() { return hasRun; }

private:
    rListBox();
    rTextDisplay *list;
    rKeyboard *keyboard;
    void (*populateFunc)(void *);

    int dialogID;
    int height;
    int width;
    int currentSelect;
    bool update;
    bool addedString;
    bool addedData;
    bool showList;
    int returnValue;
    int delayKill;
    int buttonHit;
    int okX, okY;
    int cancelX, cancelY;
    unsigned int totalCycles;
    unsigned int currentCycle;
    unsigned short int **font;
    bool hasRun;
};

#endif
