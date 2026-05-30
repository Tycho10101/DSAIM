#ifndef _USERINPUT_
#define _USERINPUT_

#include "globalDefs.h"

#define DEFAULTLINE 3
#define LIMIT 220
//#define LIMIT 178
#define NEWLINE_SPACE 255
#define NEWLINE_BREAK 254
class rUserInput
{
public:
    rUserInput();
    rUserInput(unsigned int, unsigned int);
    ~rUserInput();

    void rSetFont(unsigned short int **fnt) {font = fnt;} 
    void rClear();

    bool rMoveCursorUp();
    bool rMoveCursorDown();
    bool rMoveCursorLeft();
    bool rMoveCursorRight();
    
    int rPushKey(int);
    int rPushKeySingleLine(int, int limit = -1);
    char * rGetUnformattedBuffer() { return buffer; }
    char * rFormat();
    void rSetBuffer(char*);
    int rDraw(int x=17, int y=134);
    int rNeedsDrawn();

    void rSetPosition(int pos) { position = pos; };
    int rGetPosition() { return position; };
    void rSetPosResult(int res) { oldResult = res; };
    int rGetPosResult() { return oldResult; };
    

private:
    void rRemoveNewLines();
    void rCalcNewLines();
    int  rGetStartLine();
    
    char buffer[IBUFSIZE];
    int position;
    unsigned int currentLine;
    unsigned int oldCurrentLine;
    unsigned int maxLines;
    unsigned int width;
    unsigned int startDrawLine;
    unsigned short int **font;
    bool update;
    int oldResult;
};

#endif
