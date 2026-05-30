#include <nds.h>
#include <stdio.h>
#include <string.h>
#include <../lib/libfb/libcommon.h>
#include "keyboard.h"
#include "globalDefs.h"
#include "debug.h"
#ifdef WIFIDEBUG
#include "../lib/rTOC2/wifidebug.h"
#define WPRINT(format, args...)  rWifiDebug::rPrint("Keyboard %s :" format,__func__, ##args)
#else
#define WPRINT(format, args...)
#endif

char keybd[][16] = { "1234567890-=\0", "qwertyuiop[]\0", "asdfghjkl;'\0", "zxcvbnm,./\\\0",
                     "!@#$%^&*()_+\0", "QWERTYUIOP{}\0", "ASDFGHJKL:\"\0", "ZXCVBNM<>?|\0" };

rKeyboard::rKeyboard()
{
    font = NULL;
    update = true;
    smallCase = true;
    capsLock = false;
    keypress = false;
    keyState = 0;
    flashOn = false;
    flashState = 0;	
}

rKeyboard::~rKeyboard()
{
}

int rKeyboard::rNeedsDrawn()
{
    if (!update)
        return 0;
    return 1;
}

int rKeyboard::rDraw()
{
    if (font == NULL)
        return 0;
    int x=0, y=70, calc, pass;
    int start;
    setFont((uint16**)font);
    setColor(RGB15(0,0,0));
    if (flashOn && flashCoords[0] != 0xFFFF)
    {
        WPRINT("flashon flashcoords %d %d", flashOn, flashCoords[0]);
        bg_drawRect(flashCoords[0], flashCoords[1], flashCoords[2], flashCoords[3], RGB15(31,31,31));
        flashCoords[0] = 0xFFFF;
    }

    // The original bottom artwork supplies the chrome; we only need to draw
    // the live key legends and state overlays.
    if (smallCase)
        start = 0;
    else
        start = 4;

    pass = 0;
    for (int i=start; i<start+4; i++)
    {
        switch (pass)
        {
            case 0: x = 6; break;
            case 1: x = 15; break;
            case 2: x = 25; break;
            default: x = 34; break;
        }
        for (int j=0; j<(int)strlen(keybd[i]); j++)
        {
            calc = (15-getCharWidth(keybd[i][j]))/2;
            if ((calc % 2) == 1)
                calc++;
            bg_dispChar(x+calc,y,keybd[i][j]);
            x+=19;
        }
        y += 24;
        pass++;
    }

    if (capsLock)
        bg_drawRect(10, 127, 15, 130, RGB15(0,31,0));
    else
        bg_drawRect(10, 127, 15, 130, RGB15(21,21,21));
    update = false;
    return 1;
}

int rKeyboard::rCheckKey2(int currentScreen, unsigned short *px, unsigned short *py, bool *pickup)
{
    touchRead(&touchXY);
    secondHit = 0;
    iOldKey = (unsigned char)rCheckKey(touchXY.px, touchXY.py, currentScreen);
    swiWaitForVBlank();
    secondHit = 1;
    iKey = (unsigned char)rCheckKey(touchXY.px, touchXY.py, currentScreen);
/*    if (keyState == 0)
    {
    	iOldKey = (unsigned char)rCheckKey(touchXY.px, touchXY.py, currentScreen);
	keyState = 1;
    }
    else
    {
    	iKey = (unsigned char)rCheckKey(touchXY.px, touchXY.py, currentScreen);
        keyState = 0;
    }*/
    
    if ((iKey == iOldKey)) 
    {
        if ((iOldKey == (unsigned char)rKey::KEY_NONE))
        {
            if (flashState == 1)
            {
                flashState = 0;
                update = true;
            }
            flashOn = false;
            keypress = false;
            if ( (currentScreen == MENU_SCREEN) || (currentScreen == DIALOG_SCREEN) || (currentScreen == LISTBOX_SCREEN) )
            {
                *px = touchXY.px;
                *py = touchXY.py;
                return iKey;
            }
        }
        else
        {
            if (flashCoords[0] != 0xFFFF)
            {
                flashState = 1;
                flashOn = true;
            }
            else
            {	
                flashOn = false;
            }
        }
    }
    if (pickup)
    {
        if (*pickup == false)
            keypress = false;
    }
    if ( (iKey > 0) && (iKey < 255) && (iOldKey == iKey) && (keypress == false) )
    {
        *px = touchXY.px;
        *py = touchXY.py;
        keypress = true;
        if (iKey == (unsigned char)rKey::KEY_SHIFT)
        {
            rToggleShift();
        }
        else if (iKey ==  (unsigned char)rKey::KEY_CAPS) //caps lock
        {
            rToggleCaps();
        }
        else
        {
            if ( (smallCase == FALSE) && (capsLock == FALSE))
            {
                if (iKey == (unsigned char)rKey::KEY_NAV_DOWN)
                {
                    WPRINT("SHIFT DOWN PUSHED");
                    iKey = (unsigned char)rKey::KEY_SHIFT_DOWN;
                }
                if (iKey == (unsigned char)rKey::KEY_NAV_UP)
                    iKey = (unsigned char)rKey::KEY_SHIFT_UP;
            }       
            if (rIsShiftDown())
            {
                rToggleShift();
            }
	    update = true;
            return iKey;
        }
    }
    return rKey::KEY_NONE;
}

int rKeyboard::rCheckKey(unsigned short int x, unsigned short int y, int currentScreen)
{
    int pressed = 0;
    flashCoords[0] = 0xFFFF;
    scanKeys();
    pressed = keysHeld();
    if ( (currentScreen == LOGIN_SCREEN) || (currentScreen == EDITBOX_SCREEN) )
	return rCheckLoginKey(x,y, pressed);
    if ((currentScreen == MENU_SCREEN) || (currentScreen == DIALOG_SCREEN) || (currentScreen == LISTBOX_SCREEN) )
    {
        flashOn = false;
        return rKey::KEY_NONE;
    }
    if ( (pressed & KEY_SELECT) == KEY_SELECT )
    {
        update = true;
        flashOn = false;
        return rKey::KEY_BUTTON_SELECT;
    }
    if ( ((pressed & KEY_R) == KEY_R) || ((pressed & KEY_L) == KEY_L))
    {
	update = true;
        flashOn = false;
        return rKey::KEY_TRIGGER_R;
    }
    else if ((pressed & KEY_START) == KEY_START)
    {
        update = true;
        flashOn = false;
        return rKey::KEY_BUTTON_START;
    }
    if (currentScreen == MAIN_SCREEN)
    {
	return rCheckMainKey(x,y);
    }
    if (currentScreen == BUDDY_SCREEN)
    {
        if ((pressed & KEY_DOWN) == KEY_DOWN)
            return rKey::KEY_DIRECTION_DOWN;
        if ((pressed & KEY_UP) == KEY_UP)
            return rKey::KEY_DIRECTION_UP;
        return rCheckBuddyKey(x,y);
    }
    return rKey::KEY_NONE;
}

bool rKeyboard::rToggleShift()
{
   if (!capsLock)
   {
       smallCase = !smallCase;
       update = true;
   }
   return !smallCase;
}

bool rKeyboard::rToggleCaps()
{
    capsLock = !capsLock;
    if (capsLock)
    {
        smallCase = false;
        update = true;
    }
    else
    {
        smallCase = true;
        update = true;
    }
    return capsLock;
}

int rKeyboard::rCheckMainKey(unsigned short int x, unsigned short int y)
{
    int i, cx, lcase;
    if ( x<3 || x > 249 )
    {
        return rKey::KEY_NONE;
    }
    if ( y<3 || y>183 )
    {
        return rKey::KEY_NONE;
    }
    if (y > 159) //5th row
    {
        if (x > 61 && x < 195)
        {
            flashCoords[0] = 62;
            flashCoords[1] = 160;
            flashCoords[2] = 194;
            flashCoords[3] = 181;
            return ' ';
        }
        else
        {
            return rKey::KEY_NONE;
        }
    }
    else if (y > 136) //4th row
    {
        if (x > 239)
        {
            return rKey::KEY_NONE;
        }
        lcase = (smallCase == true) ? 3 : 7;
        cx = 222;
        for (i=10; i>=0; i--)
        {
            if (x > cx)
            {
                flashCoords[0] = cx+1;
                flashCoords[1] = 137;
                flashCoords[2] = cx+17;
                flashCoords[3] = 158;
                return keybd[lcase][i];
            }
            cx-=19;
        }
        if (x > 13)
        {
            flashCoords[0] = 15;
            flashCoords[1] = 137;
            flashCoords[2] = 15+17;
            flashCoords[3] = 158;
            return rKey::KEY_SHIFT; //SHIFT
        }
        else
        {
            return rKey::KEY_NONE;
        }
    }
    else if (y > 112) //3rd row
    {
        lcase = (smallCase == true) ? 2 : 6;
        if (x > 232)
        {
            flashCoords[0] = 234;
            flashCoords[1] = 113;
            flashCoords[2] = 234+14;
            flashCoords[3] = 133;
            return '\n';
        }
        cx = 213;
        for (i=10; i>=0; i--)
        {
            if (x > cx)
            {
                flashCoords[0] = cx+1;
                flashCoords[1] = 113;
                flashCoords[2] = cx+17;
                flashCoords[3] = 133;
                return keybd[lcase][i];
            }
            cx-=19;
        }
        if (x > 4)
        {
            flashCoords[0] = 6;
            flashCoords[1] = 113;
            flashCoords[2] = 6+17;
            flashCoords[3] = 133;
            return rKey::KEY_CAPS; //Caps lock
        }
        else
        {
            return rKey::KEY_NONE;
        }
    }
    else if (y > 88) //2nd row
    {
        lcase = (smallCase == true) ? 1 : 5;
        if (x > 239)
        {
            return rKey::KEY_NONE; 
        }
        cx = 222;
        for (i=11; i>=0; i--)
        {
            if (x > cx)
            {
                flashCoords[0] = cx+1;
                flashCoords[1] = 89;
                flashCoords[2] = cx+17;
                flashCoords[3] = 109;
                return keybd[lcase][i];
            }
            cx-=19;
        }
    }    
    else if (y > 64) //1st row
    {
        lcase = (smallCase == true) ? 0 : 4;
        if (x > 232)
        {
            flashCoords[0] = 233;
            flashCoords[1] = 65;
            flashCoords[2] = 233+17;
            flashCoords[3] = 85;
            return '\b';
        }
        cx = 213;
        for (i=11; i>=0; i--)
        {
            if (x > cx)
            {
                flashCoords[0] = cx+1;
                flashCoords[1] = 65;
                flashCoords[2] = cx+17;
                flashCoords[3] = 85;
                return keybd[lcase][i];
            }
            cx-=19;
        }
    }
    else //lets check other buttons
    {
        if (INSIDE(x, y, 184, 7, 240, 54)) //direction keys
        {
            if (INSIDE(x, y, 203, 7, 221, 30)) //UP key  -4
            {
                flashCoords[0] = 204;
                flashCoords[1] = 8;
                flashCoords[2] = 220;
                flashCoords[3] = 29;
                return rKey::KEY_NAV_UP;
            }
            else if (INSIDE(x, y, 203, 31, 221, 54)) //DOWN key  -5
            {
                flashCoords[0] = 204;
                flashCoords[1] = 32;
                flashCoords[2] = 220;
                flashCoords[3] = 53;
                return rKey::KEY_NAV_DOWN;
            }
            else if (INSIDE(x, y, 184, 31, 202, 54)) //LEFT key  -6
            {
                flashCoords[0] = 185;
                flashCoords[1] = 32;
                flashCoords[2] = 201;
                flashCoords[3] = 53;
                return rKey::KEY_NAV_LEFT;
            }
            else if (INSIDE(x, y, 222, 31, 240, 54)) //RIGHT key  -7
            {
                flashCoords[0] = 223;
                flashCoords[1] = 32;
                flashCoords[2] = 239;
                flashCoords[3] = 53;
                return rKey::KEY_NAV_RIGHT;
            }
        }
        else if(INSIDE(x, y, 130, 3, 163, 54)) //Send/warn/block
        {
            if (INSIDE(x, y, 130, 3, 163, 20)) //Send
            {
                flashCoords[0] = 131;
                flashCoords[1] = 4;
                flashCoords[2] = 162;
                flashCoords[3] = 19;
                return rKey::KEY_SEND;
            }
            else if (INSIDE(x, y, 130, 20, 163, 37)) //Warn
            {
                flashCoords[0] = 131;
                flashCoords[1] = 21;
                flashCoords[2] = 162;
                flashCoords[3] = 36;
                return rKey::KEY_WARN;
            }
            if (INSIDE(x, y, 130, 37, 163, 54)) //Block
            {
                flashCoords[0] = 131;
                flashCoords[1] = 38;
                flashCoords[2] = 162;
                flashCoords[3] = 53;
                return rKey::KEY_BLOCK;
            }
        }
        else if(INSIDE(x, y, 5, 3, 123, 54)) //chat list
        {
            if(INSIDE(x, y, 114, 3, 123, 11)) //chat list scroll up
            {
                flashCoords[0] = 114;
                flashCoords[1] = 3;
                flashCoords[2] = 123;
                flashCoords[3] = 11;
                return rKey::KEY_CHATLIST_SCROLLUP;	
            }
            else if(INSIDE(x, y, 114, 46, 123, 54)) //chat list scroll down
            {
                flashCoords[0] = 114;
                flashCoords[1] = 46;
                flashCoords[2] = 123;
                flashCoords[3] = 54;
                return rKey::KEY_CHATLIST_SCROLLDOWN;	
            }
            else if(INSIDE(x, y, 11, 2, 114, 22)) //first name
            {
                return rKey::KEY_LIST_1ST;
            }
            else if(INSIDE(x, y, 11, 22, 114, 38)) //second name
            {
                return rKey::KEY_LIST_2ND;
            }
            else if(INSIDE(x, y, 11, 38, 114, 55)) //third name
            {
                return rKey::KEY_LIST_3RD;
            }
        }
        else if(INSIDE(x, y, 241, 3, 250, 11)) //text scroll up
        {
            flashCoords[0] = 241;
            flashCoords[1] = 3;
            flashCoords[2] = 250;
            flashCoords[3] = 11;
            return rKey::KEY_TEXT_SCROLLUP;
        }
        else if(INSIDE(x, y, 241, 46, 250, 54)) //text scroll down
        {
            flashCoords[0] = 241;
            flashCoords[1] = 46;
            flashCoords[2] = 250;
            flashCoords[3] = 54;
            return rKey::KEY_TEXT_SCROLLDOWN;
        }
        else
        {
            return rKey::KEY_NONE;
        }
    }
    return rKey::KEY_NONE;
}
int rKeyboard::rCheckLoginKey(unsigned short int x, unsigned short int y, int button)
{
    if ( (button & KEY_SELECT) == KEY_SELECT)
        return rKey::KEY_BUTTON_SELECT;
    int key = rCheckMainKey(x, y);
    switch (key)
    {
        case rKey::KEY_CHATLIST_SCROLLUP:
        case rKey::KEY_CHATLIST_SCROLLDOWN:
        case rKey::KEY_LIST_1ST:
        case rKey::KEY_LIST_2ND:
        case rKey::KEY_LIST_3RD:
        case rKey::KEY_TEXT_SCROLLUP:
        case rKey::KEY_TEXT_SCROLLDOWN:
        case rKey::KEY_WARN:
        case rKey::KEY_BLOCK: return rKey::KEY_NONE; break;
        default: return key; break;
    }
    return key;
}
        


	

int rKeyboard::rCheckBuddyKey(unsigned short int x, unsigned short int y)
{
    if(!INSIDE(x, y, 16, 10, 238, 174)) //check to see if hit
        return rKey::KEY_NONE;
    return rKey::KEY_BUDDY;
}
     
void rKeyboard::rWaitForTouch()
{
    scanKeys();
    int specialKeysPressed = keysHeld();
    while ((specialKeysPressed & KEY_TOUCH) == 0)
    {
        swiWaitForVBlank();
        scanKeys();
        specialKeysPressed = keysHeld();
    }
    while ((specialKeysPressed & KEY_TOUCH) != 0)
    {
        swiWaitForVBlank();
        scanKeys();
        specialKeysPressed = keysHeld();
    }
}
