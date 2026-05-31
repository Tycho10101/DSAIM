#ifndef _TOC2_
#define _TOC2_

#include <sys/socket.h>
#include <netinet/in.h>
#include "buddy.h"

#define BUFLEN 4096
#define LISTSIZE 256
#define TIMEOUT 30
#define HTTPRETRY 3

#define IS_MSG(a) (a >= (int)PTYPE_SIGNON) && (a <= (int)PTYPE_KEEPALIVE)
enum PacketType
{
 PTYPE_SIGNON = 1,
 PTYPE_DATA,
 PTYPE_ERROR,
 PTYPE_SIGNOFF,
 PTYPE_KEEPALIVE
};

enum CONNECT_ERROR {
ERROR_NO_ERROR = 0,
ERROR_RESOLVE_IP,
ERROR_SOCKET_CREATE,
ERROR_CONNECT_FAIL,
ERROR_FAIL_LOGIN,
ERROR_PARSE_FAIL,
ERROR_SEND_FAIL,
ERROR_RETRY
};

enum {
TIMER_INITIAL,
TIMER_RUNNING_CHECK,
TIMER_STOPPED_CHECK,
TIMER_RUNNING_WAIT,
TIMER_STOPPED_WAIT
};

enum {
TIMER_STATE_NORMAL,
TIMER_STATE_PING,
TIMER_STATE_TIMEOUT,
TIMER_STATE_GOTACK
};


enum SERVER_MESSAGE_TYPES {
SERVER_ERROR_USERUNAVAILABLE,       // User not available. 901
SERVER_ERROR_CANTWARN,            // User not available to warn. 902
SERVER_ERROR_MESSAGELIMIT,        // User exceeded message limit. 903

SERVER_ERROR_INVALIDACCOUNT,       // Account not valid. 912
SERVER_ERROR_REQUEST,                // Error processing input. 913
SERVER_ERROR_SERVICEUNAVAILABLE,   // Service unavailable. 914

SERVER_ERROR_TOOFAST,               // Sending messages too fast. 960
SERVER_ERROR_MISSEDBIG,             // Missed a message because of size. 961
SERVER_ERROR_MISSEDFAST,            // Missed a message because too fast. 962

SERVER_ERROR_WRONGPASS,          // Incorrect password. 980
SERVER_ERROR_TEMPUNAVAILABLE,     // Service temporarly unavailable. 981
SERVER_ERROR_HIGHWARNING,         // Warning level too high. 982
SERVER_ERROR_CONNECTFAST,         // Trying to connect and disconnect too fast. 983
SERVER_ERROR_DISCONNECT,	  // User was disconnected
SERVER_ERROR_UNKNOWN,             // Unknown signon. 989

SERVER_ERROR_DEFAULT,              // Default error
SERVER_ERROR_MAX,                   //End of error messages

SERVER_SIGNON_SUCCESS,               //User successfully signed on
SERVER_UPDATE_BUDDY,                //Buddy list was updated (user signed on/off)
SERVER_IM_RECEIVED,                  //User received an instant message
SERVER_WARNED,                      //User got warned
SERVER_NICK,				//Got user nickname

SERVER_CONFIG_RECEIVED,              //Got a configuration
SERVER_GET_INFO,                     //Requested user info
SERVER_CHAT_JOINED,                  //Joined a chat room
SERVER_CHAT_IN,                      //Received a chat room message
SERVER_CHAT_UPDATE_BUDDY,            //Received a chat room member update
SERVER_PAUSE,                        //Server request a pause
SERVER_UNKNOWN_MESSAGE               //blank message //keep alive?
 

};

struct msgList
{
    char *data;
    int  length;
};

class rTOC2
{
public:

    rTOC2();
    ~rTOC2();
    int     rConnect(char *, char *);
    bool    rListen(SERVER_MESSAGE_TYPES&);

    void    rSetErrorHandler(void (*func)(const int, const char*))                                                { errorFunc   = func; }
    void    rSetStatusHandler(void (*func)(const int, const char*))                                               { statusFunc  = func; }
    void    rSetDebugHandler(void (*func)(const int, const char*))                                                { debugFunc   = func; }
    void    rSetBuddyUpdateHandler(void (*func)(const char*,const int, const char*, const char*, const int))      { updateFunc  = func; }
    void    rSetReceiveIMHandler(void (*func)(const char*, const char*, const bool))                              { receiveFunc = func; }
    void    rSetPauseHandler(void (*func)(void))                                                                  { pauseFunc   = func; }
    void    rSetUnPauseHandler(void (*func)(void))                                                                { unpauseFunc = func; }
    void    rSetSignOnHandler(void (*func)(void))                                                                 { signonFunc  = func; }
    void    rSetWarnedHandler(void (*func)(const char*, const int))                                               { warnedFunc  = func; }
    void    rSetNickHandler(void (func)(const char*))								{ nickFunc    = func; }
    void    rSetUnknownHandler(void (*func)(void))                                                                { unknownFunc = func; }
    void    rSetGetInfoHandler(void (*func)(const char*))                                                         { getinfoFunc = func; }
    void    rSetChatJoinHandler(void (*func)(const char*, const int))                                             { chatJoinFunc = func; }
    void    rSetReceiveChatHandler(void (*func)(const char*, const char*, const char*))                           { receiveChatFunc = func; }

    void    rSendIM(const char *, const char *, const bool away = false);
    void    rJoinChatRoom(const char *);
    void    rSendChatRoom(const int, const char *);
    void    rLeaveChatRoom(const int);
    void    rAddBuddy(const char *);
    void    rWarn(const char *, bool anonymous = false);
    void    rBlock(const char *);
    void    rSetAway(const char*);
    void    rSetAwayMessage(const char*);
    void    rGetInfo(const char*);
    int     rSimpleReadInfo(const char*);
    bool    rPollProfile();
    void    rDisconnect();

    char*   rGetFirstBuddyGroup(int&, int&, int&);
    char*   rGetNextBuddyGroup(int&, int&, int&);

    char*   rGetFirstBuddy(int, int&);
    char*   rGetNextBuddy(int&);

    bool    rIsAway() { return isAway; }
    void    rSetSpecialAway(char *);
    bool    rIsSpecialAway() { return isSpecialAway; }

    void   debugBuddy();

protected:
    static void rTimerTimeout();
    int volatile timerTick;
    int volatile timerStatus;
    int volatile timerState;
    bool volatile isDisconnected;

private:
    bool rFlapSignon();
    bool rTocLogin();
    bool rConfigureUser();
    char* rRoastPassword();
    int rSendFlap(int, char*);
    void rSetBlock(bool);
    bool rRecvFlap();
    char *rEncode(char*);
    void rCreateTimer();
    void rDestroyTimer();
    void rStartTimer(bool);
    void rStopTimer();
    
    void rLookupResponse(char *);

    void rParseUpdateMessage(char *); 
    const char *rGetChatRoomName(int);
    void rSetChatRoom(int, const char *);
    void rClearChatRoom(int);

    void rClearMessages();
    void rAddMessages(char *, int);
    void rAddMessage(struct msgList *);
    void rPopMessage();
    void rCopyMessage(char *);
    bool rMessageAvailable();
    bool rIsMessageFree();
    bool rParseResponse(char *, int&);

    bool rCheckProfile();
    bool rRetryProfile(char *);
    char* rFormatProfile();
    

    char  buffer[BUFLEN*2];
    char  awayMessage[1024];
    bool  isAway;
    bool  isSpecialAway;
    struct msgList **messageList;
    char  tocServer[30];
    int   tocPort;
    char  authServer[30];
    int   authPort;
    char  language[15];
    char  version[50];
    char  user[BUDDYLEN];
    unsigned long  userCode;
    char  pass[50];
    char  roastString[8];
    char  roastPass[50];
    short sequence;

    int   sock;
    struct sockaddr_in server;
    int receive;
    int blockarg;

    char sysMessage[100];
    SERVER_MESSAGE_TYPES sysMsgType;

    int warnPercent;
    char warnUser[BUDDYLEN];

    bool serverPaused;

    bool gotIM;
    char sourceUser[BUDDYLEN];
    char currentIM[BUFLEN];
    bool sourceAway;
    bool sourceOnline;
    unsigned int sourceWarn;
    unsigned int sourceTime;
    unsigned int sourceIdle;
    int sourceStat;
    char sourceURL[250];
    int sourceChatID;
    char sourceChatRoom[128];
  
    char currentConfig[BUFLEN*2];
    char updateBuddy[128];
    char updateChatBuddy[256];


    rBuddyList *buddyList;
    int currentBuddyGroup;
    int currentBuddy;
    
    void (*errorFunc)(const int, const char*);
    void (*statusFunc)(const int, const char*);
    void (*debugFunc)(const int, const char*);
    void (*updateFunc)(const char*, const int, const char*, const char*, const int);
    void (*receiveFunc)(const char*, const char*, const bool);
    void (*pauseFunc)(void);
    void (*unpauseFunc)(void);
    void (*signonFunc)(void);
    void (*warnedFunc)(const char*, const int);
    void (*nickFunc)(const char*);
    void (*unknownFunc)(void);
    void (*getinfoFunc)(const char*);
    void (*chatJoinFunc)(const char*, const int);
    void (*receiveChatFunc)(const char*, const char*, const char*);

    
    int httpSock;
    int httpSize;
    char httpInfo[BUFLEN]; //where the info is stored
    int  httpRetries;
    char httpURL[200];
    int  chatRoomIDs[32];
    char chatRoomNames[32][128];

};

#endif
    
