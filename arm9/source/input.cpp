#include <string.h>
#include <stdio.h>
#include "nds.h"
#include <../lib/libfb/libcommon.h>
#include "input.h"
#include <stdarg.h>
#include "globalDefs.h"
#include "debug.h"

#define ISNEWLINE(x)  ( (x == '\n') || (x == NEWLINE_BREAK) || (x == NEWLINE_SPACE) )



rUserInput::rUserInput()
{
    width = LIMIT;
    maxLines = DEFAULTLINE;
    memset(buffer, 0, IBUFSIZE);
    startDrawLine = position = currentLine = 0;
    font = NULL;
    update = true;
    oldResult = 0;
}

rUserInput::rUserInput(unsigned int lines, unsigned int w)
{
    maxLines = (lines > 1) ? lines : DEFAULTLINE;
    width = (w >= 10) ? w : LIMIT; 
    memset(buffer, 0, IBUFSIZE);
    startDrawLine = position = currentLine = 0;
    font = NULL;
    update = true;
    oldResult = 0;
}

rUserInput::~rUserInput()
{
}

void rUserInput::rClear()
{
    buffer[0] = '\0';
    startDrawLine = position = currentLine = oldResult = 0;
    update = true;
}
    
bool rUserInput::rMoveCursorUp()
{
    int i=0;
    int pass = 0;
    int count = 0;
    if (position == 0)
        return false;
    for (i=position-1; i>=0; i--)
    {
        if (ISNEWLINE(buffer[i]) || (i == 0))
            pass++;
        else if (pass == 0)
            count++;
        if (pass == 2)
        {
            break;
        }
    }
    if (pass == 2)
    {
        position = (i > 0) ? i+1: i;
        for (i = position; i<(position+count); i++)
        {
            if (ISNEWLINE(buffer[i]) || (buffer[i] == '\0'))
                break;
        }
        position = i;
        startDrawLine = rGetStartLine();
        update = true;
        return true;
    }
    return false;
}
bool rUserInput::rMoveCursorDown()
{
    int i=0;
    int count = 0;
    if (buffer[position] == '\0')
        return false;
    for (i=position-1; i>=0; i--)
    {
        if (ISNEWLINE(buffer[i]) )
            break;
        else
            count++;
    }
    for (i=position; i<(int)strlen(buffer); i++)
    {
        if (buffer[i] == '\0')   return false;
        if (ISNEWLINE(buffer[i]))
            break;
    }
    if (i == (int)strlen(buffer)) return false;
    position = i+1;
    for (i=position; i<position+count; i++)
    {
        if (ISNEWLINE(buffer[i]) || (buffer[i] == '\0') )
            break;
    }
    position = i;
    startDrawLine = rGetStartLine();
    update = true;
    return true;
}
bool rUserInput::rMoveCursorLeft()
{
    if (position > 0)
    {
        position--;
        while (buffer[position] == NEWLINE_BREAK)
            position--;
        update = true;
        startDrawLine = rGetStartLine();
        return true;
    }
    else
        return false;
}
bool rUserInput::rMoveCursorRight()
{
    if (buffer[position] != '\0')
    {
        position++;
        while (buffer[position] == NEWLINE_BREAK)
            position++;
        update = true;
        startDrawLine = rGetStartLine();
        return true;
    }
    else
        return false;
}
 
int rUserInput::rPushKeySingleLine(int key, int limit)
{
    int i;
    if (key == (unsigned char) rKey::KEY_NONE)
        return rKey::KEY_NONE;
    
        
    if (key == (unsigned char)rKey::KEY_SEND)
        return rKey::KEY_SEND; //pass keysend to the convo class
    if (strlen(buffer) >= (IBUFSIZE-20))
        return 0;
    if (key == (unsigned char)rKey::KEY_NAV_DOWN)
        return rKey::KEY_NAV_DOWN;
    if (key == (unsigned char)rKey::KEY_NAV_UP)
        return rKey::KEY_NAV_UP;
    if (key == (unsigned char)rKey::KEY_SHIFT_DOWN)
        return rKey::KEY_NAV_DOWN;
    if (key == (unsigned char)rKey::KEY_SHIFT_UP)
        return rKey::KEY_NAV_UP;
    if (key == (unsigned char)rKey::KEY_NAV_LEFT)
    {
        if (position > 0)
        {
            position--;
            update = true;
        }
       return 1;
    }
    else if (key == (unsigned char)rKey::KEY_NAV_RIGHT)
    { 
        if (position < (int)strlen(buffer))
        {
            position++;
            update = true;
        }
       return 1;
    }
    if (key < 0)
        return rKey::KEY_NONE;
    if ( (key == '\n')  )
        return '\n';
    if (key == '\b')
    {
        if (position == 0)
            return 1;
        for (i = position-1; i<=(int)strlen(buffer); i++)
        {
                buffer[i] = buffer[i+1];
        }
        position--;
        update = true;
    }
    //insert or append?
    else if (position == (int)strlen(buffer)) //append
    {
        if (limit > 0)
        {
            int len = 0;
            for (i=0; i<(int)strlen(buffer); i++)
            {
                len+=font[buffer[i]-32][0]+1;
                if (len > limit)
                    return rKey::KEY_NONE;
            }
        }
        buffer[position] = key;
        position++;
        buffer[position] = '\0';
        update = true;
    }
    else
    {   
        if (limit > 0)
        {
            int len = 0;
            for (i=0; i<(int)strlen(buffer); i++)
            {
                len+=font[buffer[i]-32][0]+1;
                if (len > limit)
                    return rKey::KEY_NONE;
            }
        }
        for (i=(int)strlen(buffer); i>=position; i--)
        {
            buffer[i+1] = buffer[i];
        }
        buffer[position] = key;
        position++;
        update = true;
    }
    return 1;
}

    
int rUserInput::rPushKey(int key)
{
    int i;
    if (key == (unsigned char)rKey::KEY_SEND)
        return rKey::KEY_SEND; //pass keysend to the convo class
    update = true;
    if (key == (unsigned char)rKey::KEY_TEXT_SCROLLUP)
        return rKey::KEY_TEXT_SCROLLUP; //pass keysend to the convo class
    if (key == (unsigned char)rKey::KEY_TEXT_SCROLLDOWN)
        return rKey::KEY_TEXT_SCROLLDOWN; //pass keysend to the convo class
    if (key == (unsigned char)rKey::KEY_CHATLIST_SCROLLDOWN)
	return rKey::KEY_CHATLIST_SCROLLDOWN;
    if (key == (unsigned char)rKey::KEY_CHATLIST_SCROLLUP)
	return rKey::KEY_CHATLIST_SCROLLUP;
    if (key == (unsigned char)rKey::KEY_LIST_1ST) //first user selected
        return rKey::KEY_LIST_1ST;
    if (key == (unsigned char)rKey::KEY_LIST_2ND) //second user selected
        return rKey::KEY_LIST_2ND;
    if (key == (unsigned char)rKey::KEY_LIST_3RD) //third user selected
        return rKey::KEY_LIST_3RD;
    if (key == (unsigned char)rKey::KEY_WARN)
        return rKey::KEY_WARN;
    if (key == (unsigned char)rKey::KEY_BLOCK)
        return rKey::KEY_BLOCK;
    if (strlen(buffer) >= (IBUFSIZE-20))
        return 0;
    if (key == (unsigned char)rKey::KEY_NAV_DOWN || key == (unsigned char)rKey::KEY_SHIFT_DOWN)
    {
       rMoveCursorDown();
       return 1;
    }
    if (key == (unsigned char)rKey::KEY_NAV_UP || key == (unsigned char)rKey::KEY_SHIFT_UP )
    {
       rMoveCursorUp(); 
       return 1;
    }
    if (key == (unsigned char)rKey::KEY_NAV_LEFT)
    {
       rMoveCursorLeft();
       return 1;
    }
    if (key == (unsigned char)rKey::KEY_NAV_RIGHT)
    { 
       rMoveCursorRight(); 
       return 1;
    }
    if (key < 0)
        return rKey::KEY_NONE;
    if ( (key == '\n') && (position > 0) ) 
    {
        if ( (buffer[position-1] == NEWLINE_SPACE) || (buffer[position-1] == NEWLINE_BREAK) )
        {
            for (i=(int)strlen(buffer); i>=(position); i--)
            {
                buffer[i+1] = buffer[i];
            }
            buffer[position] = '\n';
            buffer[position-1] = '\n';
            position++;
            update = true;
            return 1;
        }
    }
    rRemoveNewLines();
    if (key == '\b')
    {
        if (position == 0)
            return 1;
        for (i = position-1; i<=(int)strlen(buffer); i++)
        {
                buffer[i] = buffer[i+1];
        }
        position--;
        update = true;
    }
    //insert or append?
    else if (position == (int)strlen(buffer)) //append
    {
        buffer[position] = key;
        position++;
        buffer[position] = '\0';
        update = true;
    }
    else
    {   
        for (i=(int)strlen(buffer); i>=position; i--)
        {
            buffer[i+1] = buffer[i];
        }
        buffer[position] = key;
        position++;
        update = true;
    }
    rCalcNewLines();
    return 1;
}

void rUserInput::rRemoveNewLines()
{
    unsigned int i,j;
    for (i=0; i<strlen(buffer); i++)
    {
        if (buffer[i] == NEWLINE_SPACE)
        {
            buffer[i] = ' ';
        }
        else if (buffer[i] == NEWLINE_BREAK)
        {
            if (position >= (int)i)
                position--;
            for (j=i; j<strlen(buffer)-1; j++)
            {
                buffer[j] = buffer[j+1];
            }
            buffer[j] = '\0';
        }
    }
}


void rUserInput::rCalcNewLines()
{
    unsigned int i;
    unsigned int pos, length;
    int lines = 0;
    
    i = 0;
    length = 0;
    while ( i < strlen(buffer) )
    {
        if (buffer[i] == '\n')
        {
            length = 0;
            lines++;
        }
        else
            length+=font[buffer[i]-32][0]+1;
        if (length > width) //need a new line
        {
            pos = i;
            while (1)
            {
                if (buffer[pos] == ' ') //here is a good position
                {
                    buffer[pos] = NEWLINE_SPACE;
                    i = pos + 1;
                    length = font[buffer[i]-32][0];
                    lines++;
                    break;
                }
                if ( (buffer[pos] == '\n') || (pos == 0) || 
                     (buffer[pos] == NEWLINE_SPACE) || (buffer[pos] == NEWLINE_BREAK)) //this is a big ass word
                {
                    for (int bpos = (int)strlen(buffer); bpos >= (int)i; bpos--)
                    {
                        buffer[bpos+1] = buffer[bpos];
                    }
                    buffer[i] = NEWLINE_BREAK;
                    position++;
//                    if (position-1 == i)
//                        position++;
                    i = i + 1;
                    length = font[buffer[i]-32][0];
                    lines++;
                    break;
                }
                pos--;
            }
        }
        i++;
    }
    startDrawLine = rGetStartLine();
}

int rUserInput::rGetStartLine()
{
    int line=0;
    int result = 0;
    unsigned int count = 0;
    int oldStart = -1;
    for (count = 0; count<strlen(buffer); count++)
    {
       if ( (buffer[count] == '\n') || (buffer[count] == NEWLINE_SPACE) || 
            (buffer[count] == NEWLINE_BREAK) ) 
       {
            if ( (position <= (int)count) && (position >= (oldStart+1)) )
            {
                oldCurrentLine = currentLine;
                currentLine = line;
            }
            line++;
            oldStart = count;
       }
    }
    if ( (position <= (int)strlen(buffer)) && (position >= oldStart) )
    {
        oldCurrentLine = currentLine;
        currentLine = line;
    }
    if (currentLine > (oldResult+maxLines-1))
    {
        result = oldResult+1;
        oldResult = result;
        return result;
    }
    else if ((int)currentLine < oldResult)
    {
        result = oldResult-1;
        oldResult = result;
        return result;
    }
    return oldResult;
}
                
            
int rUserInput::rNeedsDrawn()
{
    if (update == false)
        return 0;
    return 2;
}
    
int rUserInput::rDraw(int x, int y)
{
    if (font == NULL)
        return 0;
    char txt[IBUFSIZE];
    int ypos = y; 
    int pos = 0;
    int startPos = 0;
    int count = 0;
    int tmpChar = 0;
   
    setFont((uint16**)font);
    setColor(RGB15(0,0,0));
//    fb_dispString(17,134,"gello");
//    return;
    while (1)
    {
        if ( (buffer[pos] == '\n') || (buffer[pos] == NEWLINE_SPACE) || 
             (buffer[pos] == NEWLINE_BREAK) || (buffer[pos] == '\0') ) 
        {

            if ( (count >= (int)startDrawLine) && (count < (int)(startDrawLine+maxLines)) )
            {
                startPos = 0;
                for (int p = pos-1; p>=0; p--)
                {
                    if ( (buffer[p] == '\n') || (buffer[p] == NEWLINE_SPACE) || 
                         (buffer[p] == NEWLINE_BREAK) )
                    {
                        startPos = p+1;
                        break;
                    }
                }
                if ( (position >=startPos) && (position <=pos) )
                {
                    int cursorLength=0;
                    for (int cursorPos = startPos; cursorPos < position; cursorPos++)
                    {
                        if (ISNEWLINE(buffer[cursorPos]))
                            cursorLength=0;
                        else
                            cursorLength+=font[buffer[cursorPos]-32][0]+1;
                    }
                    fb_drawRect(x+cursorLength, ypos, x+cursorLength+1, ypos+12, RGB15(0,0,0));
                }
                tmpChar = buffer[pos];
                buffer[pos] = '\0';
                strcpy(txt, &buffer[startPos]);
                if (!ISNEWLINE(txt[0]))
                {
                    fb_dispString(x, ypos, txt);
                }
                ypos+=font[0][1]+4;
                buffer[pos] = tmpChar;
                if (buffer[pos] == '\0')  break;
                startPos = pos+1;
            }
            count++;
            if (buffer[pos] == '\0')  break;
        }
        pos++;
    }
    return 2;
}

char * rUserInput::rFormat()
{
   char *s;
   rRemoveNewLines();
   s = new char[strlen(buffer)+1];
   strcpy(s, buffer);
   rCalcNewLines();
   return s;
}
   
void rUserInput::rSetBuffer(char *buf)
{
    strcpy(buffer, buf);
    rCalcNewLines();
    update = true;
}
 
