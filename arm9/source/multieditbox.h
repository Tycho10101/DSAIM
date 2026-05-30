#ifndef _MULTIEDITBOX_
#define _MULTIEDITBOX_
#include "keyboard.h"
#include "textdisplay.h"
#include "dialog.h"
#include "input.h"

class rMultiEditBox:public rDialog
{
public:
   rMultiEditBox(char *, char *, char *, int, int, int, int, int, unsigned short int **);
   ~rMultiEditBox();

   int rNeedsDrawn();
   int rDraw();
   int rExecute(rKeyboard *);
   int rParseKey(int);
   char *rGetText();
   void rSetText(char *);

private:
   rMultiEditBox(){};
   void rCalcPos();
   rUserInput *textInput;
   int textX;
   int textLen;
   int textLines;
   int currentPlace;
};

#endif

