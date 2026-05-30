#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "textdisplay.h"
#include "debug.h"


rTextDisplay::rTextDisplay(int w, int h)
{
    font = NULL;
    userInput = NULL;
    textAdded = scrollMode =false;
    width = (w > 4) ? w : DEFAULTWIDTH;
    height = (h > 0) ? h : DEFAULTHEIGHT;

    lines = new textline*[INITLINE];
    for (int i=0; i<INITLINE; i++)
    {
        lines[i] = new textline();
        memset(lines[i]->text, 0, MAXCHARS);
        lines[i]->ID = lines[i]->type = 99;
        lines[i]->extraData = NULL;
    }
    totalLines = INITLINE;
    currentLine = usedLines = inputPos[0] = inputPos[1] = 0;
    changed = true;
    color = 0;
}
    
rTextDisplay::rTextDisplay()
{
    font = NULL;
    textAdded = scrollMode = false;
    userInput = NULL;
    width = DEFAULTWIDTH;
    height = DEFAULTHEIGHT;

    lines = new textline*[INITLINE];
    for (int i=0; i<INITLINE; i++)
    {
        lines[i] = new textline();
        memset(lines[i]->text, 0, MAXCHARS);
        lines[i]->ID = lines[i]->type = 99;
        lines[i]->extraData = NULL;
    }
    totalLines = INITLINE;
    currentLine = usedLines = inputPos[0] = inputPos[1] = 0;
    changed = true;
    color = 0;
}

rTextDisplay::~rTextDisplay()
{
    for (unsigned int i=0; i<totalLines; i++)
    {
        delete lines[i];
    }
    delete [] lines;
    if (userInput)
    {
        delete userInput;
        userInput = NULL;
    }
}

void rTextDisplay::clearDisplay()
{
    unsigned int i;
    if (totalLines > INITLINE)
    {
        for (i=0; i<totalLines; i++)
        {
            delete lines[i];
        }
        delete [] lines;
        lines = new textline*[INITLINE];
    }
        
    for (i=0; i<totalLines; i++)
    {
        memset(lines[i]->text, 0, MAXCHARS);
        lines[i]->ID = lines[i]->type = 99;
        lines[i]->extraData = NULL;
    }
    usedLines = 0;
    totalLines = INITLINE;
    changed = true;
}

int pass = 0; 
int rTextDisplay::addText(char *txt)
{
    int k;
    int retLines = usedLines;
    changed = true;
    int startPos = 0;
    pass = 0;
    
    if (txt[0] == '\0')
        return -1;
    for (k=0; k<(int)strlen(txt); k++)
    {
        if (txt[k] == '\n')
        {
            txt[k] = '\0';
            addLine(&txt[startPos]);
            startPos = k+1;
            txt[k] = '\n';
        }
    }
    addLine(&txt[startPos]);    
    textAdded = true;
    return retLines;
}

void rTextDisplay::setFont(unsigned short **fnt)
{
    changed = true;
    font = fnt;
}

int rTextDisplay::length(char *txt, int n)
{
     if (font == NULL)
         return -1;
     int key;
     int pLen = 0;
     unsigned short *key_ptr;
     int sLen = strlen(txt);
     if ( (n > -1) && (n <= sLen) )
         sLen = n;
     for (int i=0; i<sLen; i++)
     {
          key = txt[i] - 32;
          if (key < 0)
          {
            continue;   
          }
          key_ptr = font[key];
          pLen+=key_ptr[0]+1;
     }
     return pLen;
}
void rTextDisplay::addLine(char *txt, int len)
{
    unsigned int i;
    textline **tmpLines;
    char *pch;
    pass++;
    if ( ((int)length(txt, len) <= width))
    {
        //find a line to use
        if ( usedLines >= totalLines ) //need more space
        {
            tmpLines = new textline*[totalLines+INITLINE]; //add INITLINE more lines
            for (i=0; i<totalLines+INITLINE; i++)
            {
                tmpLines[i] = new textline();
                if (i < totalLines)
                {
                    memcpy((textline*)tmpLines[i], (textline*)lines[i], sizeof(textline));
                    delete lines[i]; lines[i] = NULL;
                }
                else
                {
                    memset(tmpLines[i], 0, sizeof(textline));                    
                    tmpLines[i]->ID = tmpLines[i]->type = 99;
                    tmpLines[i]->extraData = NULL;
                }
            }
            delete [] lines;
            totalLines+=INITLINE;
            lines = tmpLines;
        }
        if (len < 0)
        {
            strcpy(lines[usedLines++]->text, txt);
        }
        else
        {
            strncpy(lines[usedLines]->text, txt, len);
            lines[usedLines]->text[len] = '\0'; 
            usedLines++;
        } 
        return;
    }
    else //need to use multiple lines
    {
        bool oneWord = true;  //a big word
        int pos;
        for (pos =strlen(txt)-1; pos>=0; pos--)
        {
            if (txt[pos] == ' ')
            {
                int nLen = 0;
                nLen = length(txt, pos);
                if (nLen <= (int)width) //found a good break point
                {
                    oneWord = false;
                    addLine(txt, pos);
                    break;
                }
            }
        }
        if (oneWord == true) //this is just a big word
        {
            int stop = 1;
            while (length(txt, stop) < width)
                 stop++;
            addLine(txt, stop-1);
            pch = &txt[stop-1];
            addLine(pch);
        }
        else
        {
            pch = &txt[pos+1];
            addLine(pch);
        }
    }
    return;
}

bool rTextDisplay::scrollDown(int pos)
{
    changed = true;
    scrollMode = true;
    currentLine+=pos;
    if (currentLine >= ((int)usedLines-height+1) )
    {
        scrollMode = false; 
        currentLine = usedLines-height;
        if (currentLine < 0) currentLine = 0;
        return false;
    }
    else
        return true;
}

bool rTextDisplay::scrollUp(int pos)
{
    changed = true;
    scrollMode = true;
    currentLine-=pos;
    if (currentLine < 0)
    {
        currentLine +=pos;
        return false;
    }
    return true;
}

void rTextDisplay::getText(char *n)
{
    changed = false;
    strcpy(n, "");
    for (unsigned int i=0; i<usedLines; i++)
    {
        strcat(n, lines[i]->text);
        strcat(n, "\n");
    }
}

void rTextDisplay::getActiveText(char *n)
{
    changed = false;
    strcpy(n, "");
    if (textAdded && !scrollMode)
        currentLine = usedLines-height;
    if (currentLine <0) currentLine = 0;
     unsigned int stop = currentLine+height;
    for (unsigned int i=currentLine; i<stop; i++)
    {
        if (i > usedLines-1)
            break;
        strcat(n, lines[i]->text);
        strcat(n, "\n");
    }
}

void rTextDisplay::setID(char *name)
{
    strncpy(id, name, 29);
    id[29] = '\0';
}

void rTextDisplay::setUserInput(char *in, int pos, int old)
{
    if (userInput != NULL)
    {
        delete [] userInput;
        userInput = NULL;
    }
    userInput = new char[strlen(in)+1];
    strcpy(userInput, in);
    inputPos[0] = pos;
    inputPos[1] = old;
}

void rTextDisplay::clearInput()
{
   if (userInput)
   {
       delete [] userInput;
       userInput = NULL;
   }
} 

char* rTextDisplay::getLine(int line)
{
    if (line <0) line = 0;
    if (line >= (int)totalLines)
        return NULL;
    return lines[line]->text;
}

void rTextDisplay::swapLine(int dst, int src)
{
    if (src <0) src = 0;
    if (src >= (int)totalLines)
        return;
    lines[dst] = lines[src];
}

void rTextDisplay::setTmp(int src)
{
    if (src <0) src = 0;
    if (src >= (int)totalLines)
        tmpLine = NULL;
    tmpLine = lines[src];
}

void rTextDisplay::swapTmp(int src)
{
    if (tmpLine == NULL)
        return;
    lines[src] = tmpLine;
}

void rTextDisplay::setColor(unsigned short int c, int line)
{
    if (line >= (int)usedLines)
        return;
    if (line <0)
        return;
    lines[line]->color = c;
}

unsigned short int rTextDisplay::getColor(int line)
{
    unsigned short int res = 0;
    if (line >= (int)usedLines)
        return res;
    if (line <0)
        return res;
    return lines[line]->color;
}

void rTextDisplay::setType(char c, int line)
{
    if (line >= (int)usedLines)
        return;
    if (line <0)
        return;
    lines[line]->type = c;
}

char rTextDisplay::getType(int line)
{
    char res = 0;
    if (line >= (int)usedLines)
        return res;
    if (line <0)
        return res;
    return lines[line]->type;
}

void rTextDisplay::setAdditional(unsigned short int c, int line)
{
    if (line >= (int)usedLines)
        return;
    if (line <0)
        return;
    lines[line]->additional = c;
}

unsigned short int rTextDisplay::getAdditional(int line)
{
    unsigned short int res = 0;
    if (line >= (int)usedLines)
        return res;
    if (line <0)
        return res;
    return lines[line]->additional;
}

void rTextDisplay::setID(char c, int line)
{
    if (line >= (int)usedLines)
        return;
    if (line <0)
        return;
    lines[line]->ID = c;
}

char rTextDisplay::getID(int line)
{
    char res = 0;
    if (line >= (int)usedLines)
        return res;
    if (line <0)
        return res;
    return lines[line]->ID;
}

void rTextDisplay::deleteLine(int line)
{
    int i;
    if (line >= (int)usedLines)
        return;
    if (line <0)
        return;
    if (lines[line] == NULL)
        return;
    memset(lines[line], 0, sizeof(textline));
    lines[line]->ID = lines[line]->type = 99;
    textline *tmp = lines[line];
    for (i=line; i<(int)totalLines-1; i++)
        lines[i] = lines[i+1];
    usedLines--;
    if (totalLines-1 >= INITLINE)
    {
        delete tmp;
        lines[i] = NULL;
        totalLines--;
    }
    else
        lines[i] = tmp;
    scrollUp(1);
    changed = true;
}

void* rTextDisplay::getExtra(int line)
{
    return (void*)lines[line]->extraData;
}

void rTextDisplay::setExtra(char *data, int line)
{
    lines[line]->extraData = data;
}

void rTextDisplay::setCurrentLine(int pos)
{
    if ( (pos+height) > getTotalLines() )
    {
        currentLine = getTotalLines() - height;
        if (currentLine < 0)
            currentLine = 0;
    }
    else
        currentLine = pos;
}
