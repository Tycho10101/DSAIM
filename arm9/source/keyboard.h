#ifndef _KEYBOARD_
#define _KEYBOARD_

class rKeyboard
{
public:
    rKeyboard();
    ~rKeyboard();
   
    void rSetFont(unsigned short int **fnt) {font = fnt; update = true;}
    int rDraw();
    int rNeedsDrawn();
    int rCheckKey(unsigned short int, unsigned short int, int);
    int rCheckKey2(int, unsigned short*, unsigned short*, bool *pickup = NULL);
    bool rToggleShift();
    bool rToggleCaps();
    void rWaitForTouch();
    bool rHasUpdated() { return update; }
    bool rIsShiftDown() { return !smallCase; }
    bool rIsCapsDown() { return capsLock; }
   

private:
    int rCheckMainKey(unsigned short int, unsigned short int);
    int rCheckLoginKey(unsigned short int, unsigned short int, int);
    int rCheckBuddyKey(unsigned short int, unsigned short int);
    unsigned short int **font;
    bool update;
    bool smallCase;
    bool capsLock;
    bool keypress;
    int keyState;
    int iKey, iOldKey, secondHit;
    unsigned short int flashCoords[4];
    bool flashOn;
    int  flashState;
    touchPosition touchXY;
}; 

#endif 
