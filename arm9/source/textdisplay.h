#ifndef _TEXTDISPLAY_
#define _TEXTDISPLAY_

#define DEFAULTWIDTH 220
#define DEFAULTHEIGHT 6
#define INITLINE 10
#define MAXCHARS 100


class rTextDisplay
{
public:
    rTextDisplay(int, int);
    rTextDisplay(rTextDisplay *, unsigned int additional = 0);
    rTextDisplay();
    ~rTextDisplay();

    void clearDisplay();
    int  addText(char *);
    bool scrollDown(int);
    bool scrollUp(int);
    void getText(char *);
    void getActiveText(char *);
    void copy(rTextDisplay *);
    void setFont(unsigned short **);
    unsigned short ** getFont() { return font;}
    char *getID() { return id; }
    void setID(char *);
    bool hasChanged() { return changed; }
    void setUserInput(char *, int, int);
    char *getUserInput() {return userInput;}
    char *getUserInput(int &pos, int &res) {pos = inputPos[0]; res = inputPos[1]; return userInput;}
    void clearInput();
    char *getLine(int);
    void swapLine(int, int); //used for sorting
    void setTmp(int); //used for sorting
    void swapTmp(int); //used for sorting
    int  getTotalLines() { return usedLines; }
    int  getCurrentLine() { return currentLine; }
    void  setCurrentLine(int);
    void setColor(unsigned short int, int );
    unsigned short int getColor(int);
    void setType(char, int );
    char getType(int);
    void setID(char, int );
    char getID(int);
    void setAdditional(unsigned short int, int );
    unsigned short int getAdditional(int);
    void deleteLine(int);

    void* getExtra(int);
    void  setExtra(char *, int);

    
private:
    int  length(char *, int n = -1);
    void addLine(char *, int len = -1);
 
    int currentLine;
    unsigned int usedLines;
    unsigned int totalLines;
    int height;
    int width;

    struct textline
    {
        char text[MAXCHARS];
        unsigned short int color;
        short int additional;
        char ID;
        char type;
        char *extraData;
    };
    textline **lines;
    textline *tmpLine;
    char *userInput;
    int inputPos[2]; //holds the user's input info
    unsigned short **font;
    char id[30];
    bool changed;
    bool textAdded;
    bool scrollMode;
    unsigned short int color;
    
};

#endif
