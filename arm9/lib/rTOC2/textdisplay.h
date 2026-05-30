#ifndef _TEXTDISPLAY_
#define _TEXTDISPLAY_

#define DEFAULTWIDTH 30
#define DEFAULTHEIGHT 4
#define INITLINE 10

class rTextDisplay
{
public:
    rTextDisplay(int, int);
    rTextDisplay();
    ~rTextDisplay();

    void clearDisplay();
    void addText(char *);
    bool scrollDown(int);
    bool scrollUp(int);
    void getText(char *);
    void getActiveText(char *);

    
private:
    void addLine(char *, int len = -1);
 
    int currentLine;
    unsigned int usedLines;
    unsigned int totalLines;
    int height;
    int width;
    
    char **lines;
    
};

#endif
