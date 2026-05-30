#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "textdisplay.h"
#include "mem.h"

rTextDisplay::rTextDisplay(int w, int h)
{
    width = (w > 4) ? w : DEFAULTWIDTH;
    height = (h > 0) ? h : DEFAULTHEIGHT;

    lines = new char*[INITLINE];
    for (int i=0; i<INITLINE; i++)
    {
        lines[i] = new char[width];
        memset(lines[i], 0, width);
    }
    totalLines = INITLINE;
    currentLine = usedLines = 0;
}
    
rTextDisplay::rTextDisplay()
{
    width = DEFAULTWIDTH;
    height = DEFAULTHEIGHT;

    lines = new char*[INITLINE];
    for (int i=0; i<INITLINE; i++)
    {
        lines[i] = new char[width];
        memset(lines[i], 0, width);
    }
    totalLines = INITLINE;
    currentLine = usedLines = 0;
}

rTextDisplay::~rTextDisplay()
{
    for (unsigned int i=0; i<totalLines; i++)
    {
        delete [] lines[i];
    }
    delete [] lines;
}

void rTextDisplay::clearDisplay()
{
    unsigned int i;
    if (totalLines > INITLINE)
    {
        for (i=0; i<totalLines; i++)
        {
            delete [] lines[i];
        }
        delete [] lines;
        lines = new char*[INITLINE];
    }
        
    for (i=0; i<totalLines; i++)
        memset(lines[i], 0, width);
    usedLines = 0;
    totalLines = INITLINE;
}

void rTextDisplay::addText(char *txt)
{
    char *pch;
    char *tLine;
    
    pch = strtok(txt, "\n\0");
    while (pch != NULL)
    {
        addLine(pch);
        tLine = pch+1;
        pch = strtok(NULL, "\n\0");
    }
}

void rTextDisplay::addLine(char *txt, int len)
{
    unsigned int i;
    char **tmpLines;
    char *pch;
    if ( ((int)strlen(txt) < width) || 
       ( (len < width) && (len > -1) ) ) //cram it on one line
    {
        //find a line to use
        if ( usedLines >= totalLines ) //need more space
        {
            tmpLines = new char*[totalLines+INITLINE]; //add INITLINE more lines
            for (i=0; i<totalLines+INITLINE; i++)
            {
                tmpLines[i] = new char[width];
                if (i < totalLines)
                {
                    strcpy(tmpLines[i], lines[i]);
                    delete [] lines[i]; lines[i] = NULL;
                }
                else
                {
                    memset(tmpLines[i], 0, width);
                }
            }
            delete [] lines;
            totalLines+=INITLINE;
            lines = tmpLines;
        }
        if (len < 0)
            strcpy(lines[usedLines++], txt);
        else
        {
            strncpy(lines[usedLines], txt, len);
            lines[usedLines][len] = '\0'; 
            usedLines++;
        } 
        return;
    }
    else //need to use multiple lines
    {
        bool oneWord = true;  //a big word
        for (i=strlen(txt); i>=0; i--)
        {
            if (txt[i] == ' ')
            {
                if (i < (unsigned)width) //found a good break point
                {
                    oneWord = false;
                    addLine(txt, i);
                    break;
                }
            }
        }
        if (oneWord == true) //this is just a big word
        {
            addLine(txt, width-1);
            pch = &txt[width-1];
            addLine(pch);
        }
        else
        {
            pch = &txt[i+1];
            addLine(pch);
        }
    }
    return;
}

bool rTextDisplay::scrollDown(int pos)
{
    currentLine+=pos;
    if (currentLine > ((int)usedLines-height+1) )
    {
        currentLine = usedLines-height+1;
        return false;
    }
    else
        return true;
}

bool rTextDisplay::scrollUp(int pos)
{
    currentLine-=pos;
    if (currentLine < 0)
    {
        currentLine = 0;
        return false;
    }
    else
        return true;
}

void rTextDisplay::getText(char *n)
{
    strcpy(n, "");
    for (unsigned int i=0; i<usedLines; i++)
    {
        strcat(n, lines[i]);
        strcat(n, "\n");
    }
}

void rTextDisplay::getActiveText(char *n)
{
    strcpy(n, "");
    unsigned int stop = currentLine+height;
    for (unsigned int i=currentLine; i<stop; i++)
    {
        if (i > usedLines-1)
            break;
        strcat(n, lines[i]);
        strcat(n, "\n");
    }
}
