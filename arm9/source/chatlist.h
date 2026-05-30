#include "textdisplay.h"
#ifndef _CHATLIST_
#define _CHATLIST_

#define CHATLIST_DEFAULTHEIGHT 3
#define CHATLIST_DEFAULTWIDTH  85

class rChatList
{
public:
    rChatList();
    rChatList(int, int);
    ~rChatList();
    
    int rDraw();
    int rNeedsDrawn();
    bool rAddUser(char*);
    bool rKillUser(char*);
    void rKillAllUsers();
    bool rScrollUp(int);
    bool rScrollDown(int);
    char *rGetUser(int);
    char *rGetCurrentUser() { return list->getLine(currentSelect); }
    bool rIsTalking(char *);
    void rSetFont(unsigned short **);
    void rSetColor(unsigned short int, char *);
    unsigned short int rGetColor(char *);
    bool rSelectUser(int);
    void rSetCurrentUser(int);
    bool rGotoUser(char *);

private:
    rTextDisplay *list;
    int height;
    int width;
    int currentSelect;
    bool update;
    bool addedUser;
    
};
    
#endif


