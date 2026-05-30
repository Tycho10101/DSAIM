#include "stdio.h"
#include "textdisplay.h"
#include "input.h"
#include "chatlist.h"
#include "buddyview.h"
#include "menu.h"
#include "dialog.h"
#include "editbox.h"
#include "multieditbox.h"
#include "listbox.h"

#ifdef USEWIFI
#include <../lib/rTOC2/toc2.h>
#endif

#define MAXCONVO    20
#define SCREEN_TOP	1
#define SCREEN_BOTTOM	2

class rConversationWindow
{
public:
    rConversationWindow();
    rConversationWindow(int);
    ~rConversationWindow();

    void rSetFont(unsigned short int **fnt);
/*    void rSetMenu(rMenu **menu) {activeMenu = menu;}
    void rSetDialog(rDialog **dialog) {activeDialog = dialog;}
    void rSetListBox(rListBox **listbox) {activeListBox = listbox;}
    void rSetEditBox(rEditBox **editbox) {activeEditBox = editbox;}
    void rSetMultiEditBox(rMultiEditBox **multieditbox) {activeMultiEditBox = multieditbox;}*/
    void rSetStatus(int stat) { currentStatus = stat; }
    int  rGetStatus() { return currentStatus; }
#ifdef USEWIFI
    void rSetTOC(rTOC2 **t) { client = t; }
#endif
    int rAddMessage(char *, char *msg = NULL, bool away = false);
    int rParseMainKey(int, bool *pickup = NULL); 
    int  rParseBuddyKey(int, int, int, bool *pickup = NULL); 
    void rKillConversation(char *);
    void rKillAllConversations();
    
    bool rScrollUp(char *, int);
    bool rScrollDown(char *, int);

    int rDraw(int);
    int rNeedsDrawn();
    void rSetActive(char *);
    void rSetOutputBuffer(char *txt) { activeText = txt; }
    bool rCurrentConvoChanged();
    bool rIsBuddyListShown() { return buddylist->rIsShown(); }
    int  rCurrentScreen();
    char *rGetLoginName() { return username; }
    void rSetLoginName(char *);
    void rSetLoginPassword(char *);
    void rMenuDone(const int, const int);
    void rDialogDone(const int, const int);
    void rListBoxDone(const int, const int);
    void rEditBoxDone(const int, const int, const char*);
    void rMultiEditBoxDone(const int, const int, const char*);
    rBuddyView *rGetBuddyList() { return buddylist;}
    rChatList *rGetChatList() { return chatList;}
    void rDisplayProfile(char *);

    /*MESSAGE HANDLERS*/
    static void rOnBuddyUpdate(const char*, const int, const char*, const char*, const int);
    static void rOnGetInfo(const char*);
    static void rOnReceiveIM(const char*, const char*, const bool);
    static void rOnNick(const char*);
    static void rOnError(const int, const char*);
    void debugMenu();

private:
    void rDoSignOff(bool);
    void rDoSendIM();
    void rDoGetInfo();
    void rDoAwayMessage();
    void rParseHTML(char *data);

#ifdef USEWIFI
    rTOC2 **client;
#endif
    rTextDisplay **convos;
    rUserInput *input;
    rChatList *chatList;
    rBuddyView *buddylist;
    int  currentConversation;
    int  maxConversations;
    int  totalConversations;
    unsigned short int **font;
    char *activeText;
    bool update;
/*    rMenu **activeMenu;
    rDialog **activeDialog;
    rListBox **activeListBox;
    rEditBox **activeEditBox;
    rMultiEditBox **activeMultiEditBox;*/

    char username[50];
    char password[50];

    int currentStatus;
};


