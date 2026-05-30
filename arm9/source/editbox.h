#ifndef _EDITBOX_
#define _EDITBOX_
#include "stdio.h"
#include "keyboard.h"
#include "textdisplay.h"
#include "dialog.h"
#include "input.h"

class rEditBox:public rDialog
{
public:
   rEditBox(char *, char *, char *, int, int, int, int, unsigned short int **);
   ~rEditBox();

   int rNeedsDrawn();
   int rDraw();
   int rExecute(rKeyboard *);
   int rParseKey(int);
   char *rGetText();
   void rSetText(char *);

private:
   rEditBox(){};
   void rCalcPos();
   rUserInput *textInput;
   int textX;
   int textLen;
   int currentPlace;
};

#endif
   
   

    
