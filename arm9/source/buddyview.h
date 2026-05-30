#include "textdisplay.h"
#ifndef _BUDDYVIEW_
#define _BUDDYVIEW_

#define BUDDYVIEW_DEFAULTHEIGHT 10
#define BUDDYVIEW_DEFAULTWIDTH  85

#define TYPE_BUDDY  1
#define TYPE_GROUP 2
#define BUDDYVIEW_BUDDY_AWAY 4
#define BUDDYVIEW_BUDDY_IDLE 8
#define BUDDYVIEW_BUDDY_ONLINE  16
#define BUDDYVIEW_BUDDY_OFFLINE 32

class rBuddyView
{
public:
    rBuddyView();
    rBuddyView(int, int);
    ~rBuddyView();

    int rDraw();
    int rNeedsDrawn();
    bool rAddBuddy(char*, char*, char* alias = NULL);
    bool rAddGroup(char*, char);
    bool rDeleteBuddy(char *); //used when buddy signs off
    void rDeleteAllBuddies();
    bool rSetAway(char *, bool);
    int rGetStats(char *);
    bool rSetIdle(char *, bool);
    bool rScrollUp(int);
    bool rScrollDown(int);
    char *rGetBuddy(int);
    char *rGetCurrentBuddy();
    void rSetFont(unsigned short **);
    int rPushKey(int , int );
    bool rSelectUser(int);
    int  rGetCurrentSelectPosition() { return currentSelect; }
    char *rGetBuddyAlias(char *);

    void rToggleShow();
    bool rIsShown() { return showList; }

private:
    bool rSortBuddy(int); //if the new buddy is out of group, move it in
    rTextDisplay *list; //this displays the data

    int height;
    int width;
    int currentSelect;
    bool update;
    bool addedUser;
    char gID;
    bool showList;
};
#endif
