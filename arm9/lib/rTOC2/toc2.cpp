#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <ctype.h>
#include "toc2.h"
#include "utils.h"
#include "textdefs.h"

#include "mem.h"
//#include "sgIP_errno.h"
#include <errno.h>
#include "wifidebug.h"
#include "nds.h"

#define BYTE1_16(x)  (char) ((x >> 8) & 0xFF)
#define BYTE2_16(x) (char) (x & 0xFF)

#define BYTE1_32(x)  (char) ((x >> 24) & 0xFF)
#define BYTE2_32(x)  (char) ((x >> 16) & 0xFF)
#define BYTE3_32(x)  (char) ((x >> 8) & 0xFF)
#define BYTE4_32(x) (char) (x & 0xFF)

#define SETBYTE(x, y, z, r) x[y] = z; r++
#define SETWORD(x, y, z, r) x[y] = BYTE1_16(z); x[y+1] = BYTE2_16(z); r+=2
#define SETDWORD(x, y, z, r) x[y] = BYTE1_32(z); x[y+1] = BYTE2_32(z); x[y+2] = BYTE3_32(z); x[y+3] = BYTE4_32(z); r+=4
#define SETSTRING(x, y, z, r) strcpy(&x[y], z); r += strlen(z)

#define DEBUGHANDLE(x)  if (debugFunc) (*debugFunc)(1, x);

//#define LOG(format, args...) fprintf(f, "TOC2: %s :" format,__func__, ##args ); fflush(f)
//#define LOGNOFLUSH(format, args...) fprintf(f, format,  ##args )
//#define LOGNOSTAT(format, args...) fprintf(f, format,  ##args ); fflush(f)

#define LOG(format, args...) 
#define LOGNOFLUSH(format, args...) 
#define LOGNOSTAT(format, args...) 
#define WPRINT(format, args...)  
//rWifiDebug::rPrint("TOC2: %s :" format,__func__, ##args)
#define WCLOSE() rWifiDebug::rClose()

namespace {

static const int kLoginHandshakeTimeoutSec = 8;

bool waitForReadable(int sock, int timeoutSec)
{
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sock, &readSet);

    timeval timeout;
    timeout.tv_sec = timeoutSec;
    timeout.tv_usec = 0;

    const int ready = select(sock + 1, &readSet, NULL, NULL, &timeout);
    return (ready > 0) && FD_ISSET(sock, &readSet);
}

}


rTOC2 *tocInstance;
bool isLogin = true;
rTOC2::rTOC2()
{
    tocInstance = this;
    memset(buffer, 0, BUFLEN-1);
    strcpy(tocServer, "aimexpress.oscar.aol.com");
    tocPort = 9898;
    strcpy(authServer, "login.oscar.aol.com");
    authPort = 5190;
    strcpy(language, "english");
    strcpy(version, "TIC:DSAIM 0.02e");;
    strcpy(user, "Ryan is the greatest!");
    strcpy(pass, "Long live the beast!");
    strcpy(roastString, "Tic/Toc");
  
    //WPRINT("TOC2 Constructor");
    
    errorFunc = NULL;
    statusFunc = NULL;
    debugFunc = NULL;
    updateFunc = NULL;
    receiveFunc = NULL;
    pauseFunc = NULL;
    unpauseFunc = NULL;
    signonFunc = NULL;
    warnedFunc = NULL;
    nickFunc = NULL;
    unknownFunc = NULL;
    getinfoFunc = NULL;
    chatJoinFunc = NULL;
    receiveChatFunc = NULL;

    receive = 0;
    isAway = isSpecialAway = false;

    sourceAway = false;
    memset(sourceUser, 0, BUDDYLEN-1);
    sourceChatID = -1;
    memset(sourceChatRoom, 0, sizeof(sourceChatRoom));
    memset(updateChatBuddy, 0, sizeof(updateChatBuddy));

    buddyList = NULL;

    //WPRINT("Creating message List");
    messageList = NULL;
    messageList = new struct msgList*[LISTSIZE];
    for (int i=0; i<LISTSIZE; i++)
        messageList[i] = NULL;
    currentBuddy = currentBuddyGroup = 0;

    timerState = TIMER_STATE_NORMAL;
    timerStatus = TIMER_INITIAL;
    timerTick = 0;
    isDisconnected = false;
    httpSock = 0;
    httpRetries = 0;
    for (int i=0; i<32; i++)
    {
        chatRoomIDs[i] = -1;
        memset(chatRoomNames[i], 0, sizeof(chatRoomNames[i]));
    }
    //WPRINT("Constructor finished");
    
}

rTOC2::~rTOC2() 
{
    //WPRINT("Destructor");
    rStopTimer();
    rDestroyTimer();
    if (buddyList != NULL)
        delete buddyList;
    //WPRINT("Clearing messages");
    rClearMessages();
    if (messageList != NULL)
        delete [] messageList;
    //WPRINT("destructor done");
}


int rTOC2::rConnect(char *userID, char *password)
{
    int size = 0;
    //Resolve host name
    LOG("entered rConnect\r\n");
    //WPRINT("entered rConnect with userID %s", userID);
    struct hostent *host;
    //WPRINT("Resolving host name");
    DEBUGHANDLE("Resolving host name");
    if ((host = gethostbyname(tocServer)) == NULL)
    {
        //WPRINT("ERROR: COULD NOT RESOLVE HOST");
    	if (errorFunc) (*errorFunc)(0xFF, ErrorText[ERROR_TEXT_RESOLVEHOST]);
        return ERROR_RESOLVE_IP;
    }
    //create the socket
    //WPRINT("Creating socket");
    DEBUGHANDLE("creating socket");
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
    {
        //WPRINT("ERROR: COULD NOT CREATE SOCKET");
    	if (errorFunc) (*errorFunc)(0xFF, ErrorText[ERROR_TEXT_SOCKETCREATE]);
        return ERROR_SOCKET_CREATE;
    }
    //Connect to TOC
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);
    WPRINT("host is %lu", *((unsigned long *) host->h_addr_list[0]));
    server.sin_port = htons(tocPort);

 
    //WPRINT("connecting");
    DEBUGHANDLE("connecting...");
    if (connect(sock, (struct sockaddr*) &server, sizeof(server)) < 0)
    {
        //WPRINT("ERROR: COULD NOT CONNECT");
    	if (errorFunc) (*errorFunc)(0xFF, ErrorText[ERROR_TEXT_CONNECTSERVER]);
        return ERROR_CONNECT_FAIL;
    }
    if (statusFunc) (*statusFunc)(1, "Connected to server...");

    //Send flap string
    //WPRINT("Sending flap on");
    sprintf(buffer, "FLAPON\r\n\r\n");
    send(sock, buffer, 10, 0);
    //WPRINT("recv flap on");
    DEBUGHANDLE("receiving flap");
    if (!waitForReadable(sock, kLoginHandshakeTimeoutSec))
    {
        close(sock);
        if (errorFunc) (*errorFunc)(0xFF, ErrorText[ERROR_TEXT_SERVERRESPOND]);
        return ERROR_CONNECT_FAIL;
    }
    size = recv(sock, buffer, BUFLEN-1,0);
    if (size < 0)
    {
        if (errno != EWOULDBLOCK)
        {
            //WPRINT("ERROR: SERVER NOT RESPONDING TO FLAPON");
            close(sock);
    	    if (errorFunc) (*errorFunc)(0xFF, ErrorText[ERROR_TEXT_SERVERRESPOND]);
            return ERROR_CONNECT_FAIL;
        }
    }
    else if (size == 0)
    {
        close(sock);
        //WPRINT("ERROR: SERVER DISCONNECTED AT FLAPON");
    	if (errorFunc) (*errorFunc)(0xFF, ErrorText[ERROR_TEXT_SERVERDISCONNECT]);
        return ERROR_CONNECT_FAIL;
    }
    //log in
    //WPRINT("got flap on");
    DEBUGHANDLE("got flap");
    strcpy(user, rUtilities::rNormalize(userID));
    strcpy(pass, password);
    //WPRINT("Normalized username is %s", user);
    sequence = 0;
    //WPRINT("flap signon");
    DEBUGHANDLE("flap signon");
    rFlapSignon();
    //WPRINT("flap signon done");
    DEBUGHANDLE("flap signon done");
    serverPaused = false;
    if (statusFunc) (*statusFunc)(1, "xLogging into AIM...");
    DEBUGHANDLE("starting log on");
    //WPRINT("rTocLogin");
    if (!rTocLogin())
    {
        return ERROR_FAIL_LOGIN;
    }
    //WPRINT("Configure user");
    DEBUGHANDLE("configuring user");
    rConfigureUser();
    if (signonFunc) (*signonFunc)();
    LOG("exiting rConnect\r\n");
    //WPRINT("done");
    DEBUGHANDLE("done...");
    rCreateTimer();
    rStartTimer(true);
    isLogin = false;
    return ERROR_NO_ERROR;
}

bool rTOC2::rFlapSignon()
{
    LOG("entered rFlapSignon\r\n");
    //WPRINT("entered rFlapSignon");
    int result = 0;
    int length = 8+strlen(user);
    int size;
    sequence++;
    memset(buffer, 0, BUFLEN-1);
    SETBYTE(buffer, 0, '*', result);
    SETBYTE(buffer, result, (char)PTYPE_SIGNON, result);
    SETWORD(buffer, result, sequence, result);
    SETWORD(buffer, result, (unsigned short) length, result);
    SETDWORD(buffer, result, (int) 1, result);
    SETWORD(buffer, result, (unsigned short) 1, result);
    SETWORD(buffer, result, (unsigned short) strlen(user), result);
    SETSTRING(buffer, result, user, result);

    size = send(sock, buffer, result, 0);
    //WPRINT("send flap sign on %s size: %d", &buffer[6], size);
    LOG("exited rFlapSignon\r\n");
    //WPRINT("exited rFlapSignon");
    return true;
}

bool rTOC2::rTocLogin()
{
    char tmpBuf[BUFLEN];
    int waitStep;
    LOG("entered rTocLogin\r\n");
    //WPRINT("entered rTocLogin");
    //Lets calculate the user code
    userCode = (int)user[0] * (int) pass[0] * 7696;
    //Roast the password
    rRoastPassword();
    
    memset(tmpBuf, 0, BUFLEN-1);
    memset(currentConfig, 0, BUFLEN-1);
    

    sprintf(tmpBuf, "toc2_login %s %d %s %s %s \"%s\" 160 US \"\" \"\" 3 0 30303 -kentucky -utf8 %ld", authServer, authPort, user,
            roastPass, language, version, userCode);

/*    sprintf(tmpBuf, "toc2_signon %s %d %s %s %s \"%s\" 160  %d", authServer, authPort, user,
            roastPass, language, version, userCode);*/

    //WPRINT("Sending sign on");
    DEBUGHANDLE("sending sign on string");
    rSendFlap((char)PTYPE_DATA, tmpBuf);
    //WPRINT("awaiting response");
    DEBUGHANDLE("awaiting response");

    for (waitStep = 0; waitStep < kLoginHandshakeTimeoutSec; ++waitStep)
    {
        if (!waitForReadable(sock, 1))
            continue;

        if (!rRecvFlap())
        {
            //WPRINT("could not recv response");
            return false;
        }

        //WPRINT("Looking up response %s", buffer);
        DEBUGHANDLE("got response");
        rLookupResponse(buffer);
        //WPRINT("Done looking up response %d", sysMsgType);
        if (sysMsgType < SERVER_ERROR_MAX)
        {
            //WPRINT("Got an error");
            if (errorFunc) (*errorFunc)((int)sysMsgType, sysMessage);
            return false;
        }
        if (sysMsgType == SERVER_SIGNON_SUCCESS)
        {
            //WPRINT("Successful sign on");
            DEBUGHANDLE("sign on success");
            serverPaused = false;
            sysMsgType = SERVER_UNKNOWN_MESSAGE;
            //WPRINT("DONE WITH rTOCLogin");
            DEBUGHANDLE("done with login");
            LOG("exited rTocLogin\r\n");
            return true;
        }
        if (sysMsgType == SERVER_PAUSE)
        {
            serverPaused = true;
        }

        memset(buffer, 0, BUFLEN-1);
        sysMsgType = SERVER_UNKNOWN_MESSAGE;
    }

    if (errorFunc) (*errorFunc)(0xFF, ErrorText[ERROR_TEXT_SERVERRESPOND]);
    return false;

    //unreachable
}

bool rTOC2::rConfigureUser()
{
    char tmpBuf[BUFLEN];
    LOG("entered rConfigureUser\r\n");
    //WPRINT("entered rConfigureUser");
    rSetBlock(true); //turn off blocking

    //We've signed on, lets notify our buddies
    sprintf(tmpBuf, "toc_add_buddy %s", user);
    //WPRINT("adding self %s", tmpBuf);
    DEBUGHANDLE("adding self");
    rSendFlap((char)PTYPE_DATA, tmpBuf);
    //WE will now try making the buddy list dynamic
/*
    while (!done)
    {
        WPRINT("waiting for conf recv");
    	DEBUGHANDLE("looking for config...");
        if (rRecvFlap() || (currentConfig[0] != 0))
        {
            WPRINT("GOT RECV %s", buffer);
    	    DEBUGHANDLE("got something...");
            if (currentConfig[0] == 0) 
            {
                exist = false;
                rLookupResponse(buffer);
                memset(buffer, 0, BUFLEN-1);
            }
            else
            {
                WPRINT("got config msg");
    	        DEBUGHANDLE("got CONFIG");
                exist = true;
                sysMsgType = SERVER_CONFIG_RECEIVED;
            }
            switch (sysMsgType)               
            {
                case  SERVER_CONFIG_RECEIVED: //we got a config (buddylist)
                {
                    WPRINT("Creating buddy list");
    	            DEBUGHANDLE("Got buddy list");
                    if (buddyList != NULL)
                    {
                        delete buddyList;
                        buddyList = NULL;
                    }
                    buddyList = new rBuddyList();
                    buddyList->createList(currentConfig);
                    done = true;
                    WPRINT("Buddy list has been created");
    	            DEBUGHANDLE("Buddy list created");
                    break;
                }
                default: break;
            }
        }
    }
*/
    //Tell server we are done initializing
//    sprintf(tmpBuf, "toc_add_buddy %s", user);
//    rSendFlap((char)PTYPE_DATA, tmpBuf);
    rSendFlap((char)PTYPE_DATA, "toc_set_caps 09461343-4C7F-11D1-8222-444553540000");  //set capabilities
    rSendFlap((char)PTYPE_DATA, "toc_add_permit ");
    rSendFlap((char)PTYPE_DATA, "toc_add_deny ");
    rSendFlap((char)PTYPE_DATA, "toc_init_done");
    LOG("exited rConfigureUser\r\n");
    //WPRINT("exiting rConfigureUser");
    DEBUGHANDLE("Done configuring user");
    rSetBlock(false); //turn off blocking
    return true;
}

char *rTOC2::rRoastPassword()
{
    unsigned int i, j;
    char hexVal[3], key;
    j = 0;
    strcpy(roastPass, "0x");
    for (i=0; i<strlen(pass); i++)
    {
        key = pass[i]^roastString[j];
        sprintf(hexVal, "%.2x", key);
        strcat(roastPass, hexVal);
        j++;
        j %= strlen(roastString);
    }
    return roastPass; 
}

int rTOC2::rSendFlap(int type, char* cmd)
{
    int length=0;
    int result=0;
    int sent=0;
    LOG("entered rSendFlap %s\r\n", cmd);
    while (serverPaused)  //server is paused, wait until it unpauses
    {
        if (rRecvFlap())
        {
            rLookupResponse(buffer);
        }
    }

    memset(buffer, 0, BUFLEN-1);

    length = strlen(cmd)+1;
    sequence++;
    SETBYTE(buffer, 0, '*', result);
    SETBYTE(buffer, result, (char) type, result);
    SETWORD(buffer, result, sequence, result);
    SETWORD(buffer, result, (unsigned short) length, result);
    SETSTRING(buffer, result, cmd, result);
    SETBYTE(buffer, result, 0, result);

    sent = send(sock, buffer, result, 0);
    buffer[0] = 0;
    LOG("exited rSendFlap\r\n");
    return sent;
}

void rTOC2::rSetBlock(bool on)
{
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0)
    {
        if (errorFunc) (*errorFunc)(1, on ? ErrorText[ERROR_TEXT_NOBLOCK] : ErrorText[ERROR_TEXT_BLOCK]);
        return;
    }

    if (on)
    {
        if (fcntl(sock, F_SETFL, flags & ~O_NONBLOCK) < 0)
	{
    	    if (errorFunc) (*errorFunc)(1, ErrorText[ERROR_TEXT_NOBLOCK]);
	}
    }
    else
    {
        if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
	{
    	    if (errorFunc) (*errorFunc)(1, ErrorText[ERROR_TEXT_BLOCK]);
	}
    }
}
bool rTOC2::rRecvFlap()
{
    int size; 
    if (rMessageAvailable())
    {
        LOG("message from list: %s\r\n", buffer);
        rCopyMessage(buffer);
        rPopMessage();
    }
    else
    {
        bool isMsg = false;
        int totalsize = 0;
        buffer[0] = 0;
        //memset(buffer, 0, BUFLEN-1);
        if (!waitForReadable(sock, 0))
            return false;
        size = recv(sock, buffer, BUFLEN-1, 0);
	if (size < 0) //we were disconnected?
        {
            if (errno != EWOULDBLOCK)
            {
                char sError[200];
                sprintf(sError, "%s[%d]...",ErrorText[ERROR_TEXT_LOSTCONNECTION], errno);
                if(errorFunc) (*errorFunc)(SERVER_ERROR_DISCONNECT, sError);
		close(sock);
                return false;
            }
            size = 0;
        }
	else if (size == 0)
        {
	    close(sock);
            if(errorFunc) (*errorFunc)(SERVER_ERROR_DISCONNECT, ErrorText[ERROR_TEXT_AIMCLOSE]);
            return false;
        }	
        totalsize = size;
        if (totalsize > 0)
        {
//            if (isLogin)
//                WPRINT("GOT MSG %s", &buffer[6]);
            LOG("REC[loop1](%d): ", size);
            for (int i=6; i<size; i++)
                LOGNOFLUSH("%c", buffer[i]);
            LOGNOSTAT("\r\n");
            if ((buffer[0] == '*') && IS_MSG(buffer[1]) ) //this a valid message
            {
                isMsg = true;
                while(rParseResponse(buffer, totalsize)) 
                {
                    isMsg = true;
                }
            }
            if (isMsg)
            {
                rCopyMessage(buffer);
                rPopMessage();            
            }
        }
        else
            return false;
    }
    LOG("exiting rRecvFlap\r\n");
    return true;
}

bool rTOC2::rParseResponse(char *buf, int &total)
{
    int tmpSize = 0;
    int msgSize = 0;
    int limit = 0;
    LOG("entering rParseResponse\r\n");
    msgSize = (unsigned char)buf[4] * 256;
    msgSize += (unsigned char)buf[5];
    if ( (total-6) < msgSize ) //We need to get more of the message
    {
        tmpSize = 0;
        LOG("Receiving next part of the message %d/%d\r\n", total, msgSize);
        while ((total-6) < msgSize)
        {
            if (!waitForReadable(sock, 1))
                return false;
            if ( ((BUFLEN*2)-total) < BUFLEN)
                limit = (BUFLEN*2)-total-1;
            else
                limit = BUFLEN-1-total;
            tmpSize = recv(sock, &buf[total], limit, 0);
            if (tmpSize > 0)
            {
                LOG("REC[loopx](%d): %s\r\n",total, &buffer[total]);
                total+=tmpSize;
            }
            else if (tmpSize < 0)
            {
                if (errno != EWOULDBLOCK)
                {
		    close(sock);
                    if(errorFunc) (*errorFunc)(SERVER_ERROR_DISCONNECT, ErrorText[ERROR_TEXT_LOSTCONNECTION]);
                    return false;
                }
            }
	    else
            {
	        close(sock);
                if(errorFunc) (*errorFunc)(SERVER_ERROR_DISCONNECT, ErrorText[ERROR_TEXT_AIMCLOSE]);
                return false;
            }	
        } //Finished receiving message
    }
    LOG("message from server: ");
    for (int i=0; i<total; i++)
        LOGNOFLUSH("%c", buffer[i]);
    LOGNOSTAT("\r\n");
    rAddMessages(buffer, msgSize+6);
    if (total > msgSize+6) //there is more messages
    {
        int i;
        for (i = msgSize; i<total; i++)
        {
            if ( (buf[i] == '*') && (IS_MSG(buf[i+1])) )
            {
                memcpy(buffer, &buf[i], total - i);
                memset(&buffer[total-i], 0, (BUFLEN*2)-1-(total-i));
                total -= i;
                LOG("entering rParseResponse(is not done)\r\n");
                return true;
            }
        }
    }
    LOG("exiting rParseResponse (done)\r\n");
    return false;
}
    
void rTOC2::rLookupResponse(char *resp)
{
    int pos = 0;
    char error[5];
    char parser[30];
    int errorNo;

    if (strncmp(resp, "SIGN_ON:", 8) == 0)
    {
        strcpy(sysMessage, SystemMiscText[SYSTEM_MISC_TEXT_SUCCESS]);
        sysMsgType = SERVER_SIGNON_SUCCESS;
    }
    else if (strncmp(resp, "GOTO_URL:", 9) == 0)
    {
        char *pch;
        if (httpSock == 0)
        {
            pos = 9;
            strcpy(sysMessage, SystemMiscText[SYSTEM_MISC_TEXT_USERINFO]);
            sysMsgType = SERVER_GET_INFO;
            pch = strtok(&resp[pos], ":");
            pch = strtok(NULL, "\0");
            sprintf(sourceURL, "http://%s:%d/", tocServer, tocPort); 
            if (pch == NULL) {}//PARSE ERROR
            strcat(sourceURL, pch);
        }
    }
    else if (strncmp(resp, "PAUSE", 5) == 0)  //TOC needs to pause
    {
        strcpy(sysMessage, SystemMiscText[SYSTEM_MISC_TEXT_PAUSE]);
        sysMsgType = SERVER_PAUSE;
    }
    else if (strncmp(resp, "UPDATE_BUDDY2:", 14) == 0) //update buddy list
    {
        pos = 14;
        strcpy(updateBuddy, &resp[pos]);
        strcpy(sysMessage, SystemMiscText[SYSTEM_MISC_TEXT_BUDDYUPDATE]);
        sysMsgType = SERVER_UPDATE_BUDDY;
        if (resp[pos] == 0)
            memset(updateBuddy, 0, BUFLEN-1);
    }
    else if (strncmp(resp, "NICK:", 5) == 0) //get user nickname
    {
        pos = 5;
        strcpy(user, &resp[pos]);
        strcpy(sysMessage, SystemMiscText[SYSTEM_MISC_TEXT_GOTUSERNAME]);
        sysMsgType = SERVER_NICK;
    }
    else if (strncmp(resp, "CONFIG2:", 8) == 0) //here is my config
    {
        if (currentConfig[0] == 0)
        {
            pos = 8;
            strcpy(currentConfig, &resp[pos]);
            strcpy(sysMessage, SystemMiscText[SYSTEM_MISC_TEXT_GOTBUDDYLIST]);
            sysMsgType = SERVER_CONFIG_RECEIVED; 
            if (resp[pos] == 0)
            {
                memset(currentConfig, 0, BUFLEN-1);
            }
        }
    }
    else if (strncmp(resp, "IM_IN2:", 7) == 0) //received and IM
    {
        WPRINT("%s", resp);
        char *pch;
        pos = 7;
        pch = strtok(&resp[pos], ":");
        strcpy(sourceUser, pch);

        pch = strtok(NULL, ":");
        if (pch == NULL) {}//PARSE ERROR

        if (pch[0] == 'T')
            sourceAway = true; 
        else
            sourceAway = false;
        memset(currentIM, 0, BUFLEN-1);
        pch = strtok(NULL, "\0");
        if (pch == NULL) //PARSE ERROR
        strcpy(currentIM, pch);
        strcpy(sysMessage, SystemMiscText[SYSTEM_MISC_TEXT_GOTIM]);
        sysMsgType = SERVER_IM_RECEIVED;
    } 
    else if (strncmp(resp, "IM_IN_ENC2:", 11) == 0) //received and IM
    {
        WPRINT("%s", resp);
        char *pch;
        pos = 11;
        pch = strtok(&resp[pos], ":");
        
        strcpy(sourceUser, pch);

        pch = strtok(NULL, ":");
        if (pch == NULL) {} //PARSE ERROR

        if (pch[0] == 'T')
            sourceAway = true; 
        else
            sourceAway = false;
        
        pch = strtok(NULL, ":");
        pch = strtok(NULL, ":");
        pch = strtok(NULL, ":");
        pch = strtok(NULL, ":");
        pch = strtok(NULL, ":");
        pch = strtok(NULL, ":");

        memset(currentIM, 0, BUFLEN-1);
        pch = strtok(NULL, "\0");
        if (pch == NULL) {}//PARSE ERROR
        strcpy(currentIM, pch);

        strcpy(sysMessage, SystemMiscText[SYSTEM_MISC_TEXT_GOTIM] );
        sysMsgType = SERVER_IM_RECEIVED;
    } 
    else if (strncmp(resp, "CHAT_JOIN:", 10) == 0)
    {
        char *pch;
        pos = 10;
        pch = strtok(&resp[pos], ":");
        if (pch == NULL) return;
        sourceChatID = atoi(pch);

        pch = strtok(NULL, "\0");
        if (pch == NULL) return;
        strncpy(sourceChatRoom, pch, sizeof(sourceChatRoom) - 1);
        sourceChatRoom[sizeof(sourceChatRoom) - 1] = '\0';
        rSetChatRoom(sourceChatID, sourceChatRoom);
        strcpy(sysMessage, sourceChatRoom);
        sysMsgType = SERVER_CHAT_JOINED;
    }
    else if (strncmp(resp, "CHAT_IN:", 8) == 0)
    {
        char *pch;
        pos = 8;
        pch = strtok(&resp[pos], ":");
        if (pch == NULL) return;
        sourceChatID = atoi(pch);

        pch = strtok(NULL, ":");
        if (pch == NULL) return;
        strncpy(sourceUser, pch, sizeof(sourceUser) - 1);
        sourceUser[sizeof(sourceUser) - 1] = '\0';

        pch = strtok(NULL, ":");
        if (pch == NULL) return;

        pch = strtok(NULL, "\0");
        if (pch == NULL) return;
        strncpy(currentIM, pch, sizeof(currentIM) - 1);
        currentIM[sizeof(currentIM) - 1] = '\0';

        const char *roomName = rGetChatRoomName(sourceChatID);
        if (roomName != NULL)
        {
            strncpy(sourceChatRoom, roomName, sizeof(sourceChatRoom) - 1);
            sourceChatRoom[sizeof(sourceChatRoom) - 1] = '\0';
        }
        else
        {
            sprintf(sourceChatRoom, "Chat %d", sourceChatID);
        }
        strcpy(sysMessage, sourceChatRoom);
        sysMsgType = SERVER_CHAT_IN;
    }
    else if (strncmp(resp, "CHAT_IN_ENC:", 12) == 0)
    {
        char *pch;
        pos = 12;
        pch = strtok(&resp[pos], ":");
        if (pch == NULL) return;
        sourceChatID = atoi(pch);

        pch = strtok(NULL, ":");
        if (pch == NULL) return;
        strncpy(sourceUser, pch, sizeof(sourceUser) - 1);
        sourceUser[sizeof(sourceUser) - 1] = '\0';

        pch = strtok(NULL, ":");
        if (pch == NULL) return;

        pch = strtok(NULL, ":");
        if (pch == NULL) return;

        pch = strtok(NULL, ":");
        if (pch == NULL) return;

        pch = strtok(NULL, "\0");
        if (pch == NULL) return;
        strncpy(currentIM, pch, sizeof(currentIM) - 1);
        currentIM[sizeof(currentIM) - 1] = '\0';

        const char *roomName = rGetChatRoomName(sourceChatID);
        if (roomName != NULL)
        {
            strncpy(sourceChatRoom, roomName, sizeof(sourceChatRoom) - 1);
            sourceChatRoom[sizeof(sourceChatRoom) - 1] = '\0';
        }
        else
        {
            sprintf(sourceChatRoom, "Chat %d", sourceChatID);
        }
        strcpy(sysMessage, sourceChatRoom);
        sysMsgType = SERVER_CHAT_IN;
    }
    else if (strncmp(resp, "CHAT_UPDATE_BUDDY:", 18) == 0)
    {
        pos = 18;
        strncpy(updateChatBuddy, &resp[pos], sizeof(updateChatBuddy) - 1);
        updateChatBuddy[sizeof(updateChatBuddy) - 1] = '\0';
        sysMsgType = SERVER_CHAT_UPDATE_BUDDY;
    }

    else if (strncmp(resp, "EVILED:", 7) == 0)  //aah, we've been warned
    {
        char *pch;
        pos = 7;
        pch = strtok(&resp[pos], ":");
        strcpy(parser, pch);
        warnPercent = atoi(parser);
        pch = strtok(NULL, "\0");
        strcpy(warnUser, pch);
        if (strlen(warnUser) <= 1 )
            strcpy(warnUser, WarnText[WARN_TEXT_USER]);
        sprintf(sysMessage, "%s %s. %s %d", WarnText[WARN_TEXT_INFO], warnUser, WarnText[WARN_TEXT_LEVEL], warnPercent);
        sysMsgType = SERVER_WARNED;
    }    
    else if (strncmp(resp, "ERROR:", 6) == 0)  //This is a response error
    {
        pos = 6;
        strncpy(error, &resp[pos], 3); 
        errorNo = atoi(error);  //this is our error no
        switch (errorNo)
        {
            case 901: strcpy(sysMessage, SystemErrorText[SYSTEM_ERROR_TEXT_UNAVAILABLE]); sysMsgType = SERVER_ERROR_USERUNAVAILABLE; break;
            case 902: strcpy(sysMessage, SystemErrorText[SYSTEM_ERROR_TEXT_NOWARN]); sysMsgType = SERVER_ERROR_CANTWARN; break;
            case 903: strcpy(sysMessage, SystemErrorText[SYSTEM_ERROR_TEXT_MESSAGELIMIT]); sysMsgType = SERVER_ERROR_MESSAGELIMIT; break;

            case 912: strcpy(sysMessage, SystemErrorText[SYSTEM_ERROR_TEXT_ACCOUNT]); sysMsgType = SERVER_ERROR_USERUNAVAILABLE; break;
            case 913: strcpy(sysMessage, SystemErrorText[SYSTEM_ERROR_TEXT_PROCESS]); sysMsgType = SERVER_ERROR_REQUEST; break;
            case 914: strcpy(sysMessage, SystemErrorText[SYSTEM_ERROR_TEXT_SERVICEDOWN]); sysMsgType = SERVER_ERROR_SERVICEUNAVAILABLE; break;

            case 960: strcpy(sysMessage, SystemErrorText[SYSTEM_ERROR_TEXT_TOOFAST]); sysMsgType = SERVER_ERROR_TOOFAST; break;
            case 961: strcpy(sysMessage, SystemErrorText[SYSTEM_ERROR_TEXT_MISSBIG]); sysMsgType = SERVER_ERROR_MISSEDBIG; break;
            case 962: strcpy(sysMessage, SystemErrorText[SYSTEM_ERROR_TEXT_MISSFAST]); sysMsgType = SERVER_ERROR_MISSEDFAST; break;
        
            case 980: strcpy(sysMessage, SystemErrorText[SYSTEM_ERROR_TEXT_BADLOGIN]); sysMsgType = SERVER_ERROR_WRONGPASS; break;
            case 981: strcpy(sysMessage, SystemErrorText[SYSTEM_ERROR_TEXT_SERVICETEMPDOWN]); sysMsgType = SERVER_ERROR_TEMPUNAVAILABLE; break;
            case 982: strcpy(sysMessage, SystemErrorText[SYSTEM_ERROR_TEXT_WARNINGHIGH]); sysMsgType = SERVER_ERROR_HIGHWARNING; break;
            case 983: strcpy(sysMessage, SystemErrorText[SYSTEM_ERROR_TEXT_CONNECTFAST]); sysMsgType = SERVER_ERROR_CONNECTFAST; break;
            case 989: strcpy(sysMessage, SystemErrorText[SYSTEM_ERROR_TEXT_UNKNOWNERROR]); sysMsgType = SERVER_ERROR_UNKNOWN; break;
            
            default:  strcpy(sysMessage, SystemErrorText[SYSTEM_ERROR_TEXT_NOTDEFINED]); sysMsgType = SERVER_ERROR_DEFAULT; break;
        }
    }
    else
    {
        strcpy(sysMessage, SystemErrorText[SYSTEM_ERROR_TEXT_UNKNOWNCOMMAND]);
        sysMsgType = SERVER_UNKNOWN_MESSAGE;
    }
    LOG("exiting rLookupResponse\r\n");
}    

void rTOC2::rParseUpdateMessage(char *msg)
{
    char *pch;

    if (msg[0] == 0)
        return;
    LOG("entering rParseUpdateMessage\r\n");
    memset(sourceUser, 0, BUDDYLEN-1);
    pch = strtok(msg, ":"); 
    strcpy(sourceUser, pch);
    pch = strtok(NULL, ":");
    if (pch[0] == 'T') //user is online
        sourceOnline = true;
    else
        sourceOnline = false;
    pch = strtok(NULL, ":");
    sourceWarn = atoi(pch);
    pch = strtok(NULL, ":");
    sourceTime = atoi(pch);
    pch = strtok(NULL, ":");
    sourceIdle = atoi(pch);
    pch = strtok(NULL, "\0");
    if (pch[2] == 'U') sourceAway = true; else sourceAway = false;
    sourceStat = BUDDY_UNAVAILABLE;
    if (sourceOnline)
    {
        sourceStat = BUDDY_AVAILABLE;
        if (sourceAway) sourceStat |= (int)BUDDY_AWAY;
        if (sourceIdle > 0) sourceStat |= (int)BUDDY_IDLE;	
        if (pch[1] == 'C')  sourceStat |= (int)BUDDY_CELL;
    }
    if (buddyList)
        buddyList->setBuddyStat(sourceUser, sourceStat);
    LOG("exiting rParseUpdateMessage\r\n");
}

const char *rTOC2::rGetChatRoomName(int roomID)
{
    for (int i=0; i<32; i++)
    {
        if (chatRoomIDs[i] == roomID)
            return chatRoomNames[i];
    }
    return NULL;
}

void rTOC2::rSetChatRoom(int roomID, const char *roomName)
{
    int emptySlot = -1;
    for (int i=0; i<32; i++)
    {
        if (chatRoomIDs[i] == roomID)
        {
            strncpy(chatRoomNames[i], roomName, sizeof(chatRoomNames[i]) - 1);
            chatRoomNames[i][sizeof(chatRoomNames[i]) - 1] = '\0';
            return;
        }
        if (emptySlot == -1 && chatRoomIDs[i] == -1)
            emptySlot = i;
    }
    if (emptySlot != -1)
    {
        chatRoomIDs[emptySlot] = roomID;
        strncpy(chatRoomNames[emptySlot], roomName, sizeof(chatRoomNames[emptySlot]) - 1);
        chatRoomNames[emptySlot][sizeof(chatRoomNames[emptySlot]) - 1] = '\0';
    }
}

void rTOC2::rClearChatRoom(int roomID)
{
    for (int i=0; i<32; i++)
    {
        if (chatRoomIDs[i] == roomID)
        {
            chatRoomIDs[i] = -1;
            chatRoomNames[i][0] = '\0';
            return;
        }
    }
}


bool rTOC2::rListen(SERVER_MESSAGE_TYPES& msg)
{
    if (isDisconnected) //we got disconnected
    {
        close(sock);
        if(errorFunc) (*errorFunc)(SERVER_ERROR_DISCONNECT, ErrorText[ERROR_TEXT_LOSTCONNECTION]);
        return false;
    }
            
    if (rRecvFlap())
    {
        LOG("entering rListen\r\n");
        rLookupResponse(buffer);
        switch (sysMsgType)               
        {
            case SERVER_UPDATE_BUDDY: //buddy list updated
            {
                char *sGroup;
                int nGroupID = 0;
                rParseUpdateMessage(updateBuddy);
                //check if this is our server response msg
                if (!rUtilities::rNormalizeCompare(sourceUser, user) && (timerStatus == TIMER_RUNNING_WAIT) ) //this is us
                {
                    if ( (sourceStat & BUDDY_AWAY) == BUDDY_AWAY)
                        isAway = true;
                    else
                        isAway = false;
                    timerState = TIMER_STATE_GOTACK;
                }
		if (!buddyList)
                {
                    // Open OSCAR Server may send self-presence updates immediately after
                    // toc_init_done even when no TOC config/buddy list has been provided.
                    // DSAIM doesn't need to surface that startup self-update, and skipping it
                    // avoids driving the old UI/buddy path with incomplete TOC config state.
                    if (!rUtilities::rNormalizeCompare(sourceUser, user))
                        break;
                    if (updateFunc)(*updateFunc)(NULL, 100, sourceUser, NULL, sourceStat);
                    break;
                }
                sGroup = buddyList->getBuddyGroups(sourceUser, &nGroupID);
                if (updateFunc) (*updateFunc)(sGroup, nGroupID, sourceUser, buddyList->getBuddyAlias(nGroupID, sourceUser), sourceStat);
                while ( (sGroup=buddyList->getBuddyGroups(NULL, &nGroupID)) != NULL)
                {
                    if (updateFunc) (*updateFunc)(sGroup, nGroupID, sourceUser, buddyList->getBuddyAlias(nGroupID, sourceUser), sourceStat);
                }
                break;
            }
            case  SERVER_CONFIG_RECEIVED: //we got a config (buddylist)
            {
                //WPRINT("Creating buddy list");
//                DEBUGHANDLE("Got buddy list");
                if (buddyList != NULL)
                {
                    delete buddyList;
                    buddyList = NULL;
                }
                buddyList = new rBuddyList();
                buddyList->createList(currentConfig);
                //WPRINT("Buddy list has been created");
//                DEBUGHANDLE("Buddy list created");
                break;
            }
            case SERVER_GET_INFO: //Got info
            {
		strcpy(httpURL, sourceURL);
                httpRetries = 0;
                while (1)
                {
                    if (rSimpleReadInfo(sourceURL) != ERROR_RETRY)
                        break;
                }
                break;
            }
            case SERVER_PAUSE: //Server is paused
            {
                serverPaused = true;
                if (pauseFunc) (*pauseFunc)();
                break;
            }
            case SERVER_SIGNON_SUCCESS:
            {
                serverPaused = false;
                rConfigureUser();
                if (unpauseFunc) (*unpauseFunc)();
                break;
            }
            case SERVER_WARNED:
            {
                if (warnedFunc) (*warnedFunc)(warnUser, warnPercent);
                break;
            }
            case SERVER_NICK:
            {
		char *sGroup;
                char *sAlias;
		char theName[100];
		int   sID = -1;
		strcpy(theName, user);
                if (!buddyList)
                {
                    if (nickFunc) (*nickFunc)(theName);
                    break;
                }
		sGroup = buddyList->getBuddyGroups(user, &sID);
		while (sGroup != NULL)
		{
		    sAlias = buddyList->getBuddyAlias(sID, user);
                    if (sAlias != NULL)
                    {
                        strcpy(theName, sAlias);
                        break;
                    }
                    else
		        sGroup = buddyList->getBuddyGroups(NULL, &sID);
                }
                if (nickFunc) (*nickFunc)(theName);
                break;
            }
            case SERVER_IM_RECEIVED:
            {
                char *Alias;
                char nameAndAlias[200];
		int  id;
                strcpy(nameAndAlias, sourceUser);
                if (!buddyList)
                {
                    if (receiveFunc) (*receiveFunc)(nameAndAlias, currentIM, sourceAway); 
                    if (isAway)
                        rSendIM(sourceUser, awayMessage, true);
                    break;
                }
		if (buddyList->getBuddyGroups(sourceUser, &id) != NULL)
		{
                    Alias = buddyList->getBuddyAlias(id, sourceUser);
                    if (Alias != NULL)
                    {
                        sprintf(nameAndAlias, "%s:%s", sourceUser, Alias);
                    }
                }   
                if (receiveFunc) (*receiveFunc)(nameAndAlias, currentIM, sourceAway); 
                if (isAway)
                    rSendIM(sourceUser, awayMessage, true);
                break;
            }
            case SERVER_CHAT_JOINED:
            {
                if (chatJoinFunc)
                    (*chatJoinFunc)(sourceChatRoom, sourceChatID);
                break;
            }
            case SERVER_CHAT_IN:
            {
                if (receiveChatFunc)
                    (*receiveChatFunc)(sourceChatRoom, sourceUser, currentIM);
                break;
            }
            case SERVER_CHAT_UPDATE_BUDDY:
            {
                break;
            }
            case SERVER_UNKNOWN_MESSAGE:
            {
                if (unknownFunc) (*unknownFunc)();
                break;
            }
            case SERVER_ERROR_USERUNAVAILABLE:       
            case SERVER_ERROR_CANTWARN:            
            case SERVER_ERROR_MESSAGELIMIT:        
            case SERVER_ERROR_INVALIDACCOUNT:       
            case SERVER_ERROR_REQUEST:                
            case SERVER_ERROR_SERVICEUNAVAILABLE:   
            case SERVER_ERROR_TOOFAST:               
            case SERVER_ERROR_MISSEDBIG:             
            case SERVER_ERROR_MISSEDFAST:            
            case SERVER_ERROR_WRONGPASS:          
            case SERVER_ERROR_TEMPUNAVAILABLE:     
            case SERVER_ERROR_HIGHWARNING:         
            case SERVER_ERROR_CONNECTFAST:         
            case SERVER_ERROR_UNKNOWN:             
            case SERVER_ERROR_DEFAULT:
            {
                if(errorFunc) (*errorFunc)((int)sysMsgType, sysMessage);
                break;
            }
            default: break;
        }
        memset(buffer, 0, BUFLEN-1);
        sysMsgType = SERVER_UNKNOWN_MESSAGE; 
        LOG("exiting rListen\r\n");
    }
    //Check our timers
    if (timerState == TIMER_STATE_PING) //we need to send out a message to the server
    {
        char tmpMsg[200];
        timerState = TIMER_STATE_NORMAL;
        sprintf(tmpMsg, "toc_get_status %s", rUtilities::rNormalize(user));
        rSendFlap(PTYPE_DATA, tmpMsg);
        timerStatus = TIMER_RUNNING_WAIT;
    } 
    //Check to see if a profile is in
    if (httpSock != 0)
        rPollProfile();
    return false;
}

void rTOC2::rClearMessages()
{
    int i;
    for (i=0; i<LISTSIZE; i++)
    {
        if (messageList[i] != NULL)
        {
            if (messageList[i]->data != NULL)
                delete messageList[i]->data;
            delete messageList[i];
        }
        messageList[i] = NULL;
    }
}

void rTOC2::rAddMessages(char *buf, int size)
{
    int i, j;
    struct msgList *n;
    int msgSize = 0;
    if ( (size < 0) || (buf == NULL) )
        return;
    if (buf[0] == 0)
        return;
    LOG("entering rAddMessages\r\n");
    j = 0;
    for (i=0; i<size; i++)
    {
        if (buf[i] == '*')
        { 
            if (IS_MSG(buf[i+1])) //a message!
            {
                LOG("is a message!\r\n");
                j = i;
                //get the message size
                msgSize = (unsigned char)buf[i+4] * 256;
                msgSize += (unsigned char)buf[i+5];
               //Check room in buffer
                if(rIsMessageFree())
                {
                    n = new struct msgList;
                    n->length = msgSize;
                    n->data = new char[msgSize+1];
                    strncpy(n->data, &buffer[j+6], msgSize);
                    n->data[msgSize] = '\0';
                    i+=msgSize+5;
                    j = i+1;
                    LOG("Adding Message (%d) %s\r\n", n->length, n->data);
                    rAddMessage(n);
                }
                else
                {
                    i+=msgSize+5;
                    j=i+1;
                }
            }
        }
    }
    LOG("exiting rAddMessages\r\n");
}              

void rTOC2::rAddMessage(struct msgList *msg)
{
    int i=0;
    while (messageList[i] != NULL)
    {
        i++;
    }
    messageList[i] = msg;
}

void rTOC2::rPopMessage()
{
    LOG("entering rPopMessage\r\n");
    if (messageList[0] != NULL)
    {
        if (messageList[0]->data != NULL)
        {
            delete []messageList[0]->data;
        }
        delete messageList[0];
    }
    for (int i=0; i<LISTSIZE-1; i++)
    {
        messageList[i] = messageList[i+1];
    }
    messageList[LISTSIZE-1] = NULL;
    LOG("exiting rPopMessage\r\n");
}

void rTOC2::rCopyMessage(char *buf)
{
    strcpy(buf, messageList[0]->data);
}

bool rTOC2::rMessageAvailable()
{
    if (messageList[0] != NULL)
    {
        if (messageList[0]->data != NULL)
            return true;
        else  //garbage in the list??
        {
            rPopMessage();
            return rMessageAvailable();
        }
        
    }
    return false;
}

bool rTOC2::rIsMessageFree()
{
    for (int i=0; i<LISTSIZE; i++)
    {
        if(messageList[i] == NULL)
        {
            return true;
        }
    }
    return false;
}
 
void rTOC2::rSendIM(const char *userName, const char *msg, const bool away)
{
    char username[BUDDYLEN];

    strcpy(currentIM, msg);
    strcpy(username, userName);
    sprintf(buffer, "toc_send_im %s \"%s\"", rUtilities::rNormalize(username), rEncode(currentIM));
    if (away)
        strcat(buffer, " \"auto\"");
    strcpy(currentIM, buffer);
    if (rSendFlap(PTYPE_DATA, currentIM) == -1)
    {
        if (errno != EWOULDBLOCK)
        {
            close(sock);
    	    if (errorFunc) (*errorFunc)(0xFF, ErrorText[ERROR_TEXT_LOSTCONNECTION]);
        }
    }
    if (isAway)
    {
//        rSetAway(awayMessage);
    }
}

void rTOC2::rJoinChatRoom(const char *roomName)
{
    if (roomName == NULL || roomName[0] == '\0')
        return;

    strncpy(currentIM, roomName, sizeof(currentIM) - 1);
    currentIM[sizeof(currentIM) - 1] = '\0';
    sprintf(buffer, "toc_chat_join 4 \"%s\"", rEncode(currentIM));
    strcpy(currentIM, buffer);
    if (rSendFlap(PTYPE_DATA, currentIM) == -1)
    {
        if (errno != EWOULDBLOCK)
        {
            close(sock);
            if (errorFunc) (*errorFunc)(0xFF, ErrorText[ERROR_TEXT_LOSTCONNECTION]);
        }
    }
}

void rTOC2::rSendChatRoom(const int roomID, const char *msg)
{
    if (roomID < 0 || msg == NULL || msg[0] == '\0')
        return;

    strncpy(currentIM, msg, sizeof(currentIM) - 1);
    currentIM[sizeof(currentIM) - 1] = '\0';
    sprintf(buffer, "toc_chat_send %d \"%s\"", roomID, rEncode(currentIM));
    strcpy(currentIM, buffer);
    if (rSendFlap(PTYPE_DATA, currentIM) == -1)
    {
        if (errno != EWOULDBLOCK)
        {
            close(sock);
            if (errorFunc) (*errorFunc)(0xFF, ErrorText[ERROR_TEXT_LOSTCONNECTION]);
        }
    }
}

void rTOC2::rLeaveChatRoom(const int roomID)
{
    if (roomID < 0)
        return;

    sprintf(buffer, "toc_chat_leave %d", roomID);
    strcpy(currentIM, buffer);
    if (rSendFlap(PTYPE_DATA, currentIM) == -1)
    {
        if (errno != EWOULDBLOCK)
        {
            close(sock);
            if (errorFunc) (*errorFunc)(0xFF, ErrorText[ERROR_TEXT_LOSTCONNECTION]);
        }
    }
    rClearChatRoom(roomID);
}

void rTOC2::rAddBuddy(const char *userName)
{
    char username[BUDDYLEN];

    strcpy(username, userName);
    sprintf(buffer, "toc_add_buddy %s", rUtilities::rNormalize(username));
    strcpy(currentIM, buffer);
    if (rSendFlap(PTYPE_DATA, currentIM) == -1)
    {
        if (errno != EWOULDBLOCK)
        {
            close(sock);
            if (errorFunc) (*errorFunc)(0xFF, ErrorText[ERROR_TEXT_LOSTCONNECTION]);
        }
    }
}

void rTOC2::rWarn(const char *userName, bool anonymous)
{
    char username[BUDDYLEN];

    strcpy(username, userName);
    sprintf(buffer, "toc_evil %s %s", rUtilities::rNormalize(username), anonymous ? "anon" : "norm");
    strcpy(currentIM, buffer);
    if (rSendFlap(PTYPE_DATA, currentIM) == -1)
    {
        if (errno != EWOULDBLOCK)
        {
            close(sock);
            if (errorFunc) (*errorFunc)(0xFF, ErrorText[ERROR_TEXT_LOSTCONNECTION]);
        }
    }
}

void rTOC2::rBlock(const char *userName)
{
    char username[BUDDYLEN];

    strcpy(username, userName);
    sprintf(buffer, "toc_add_deny %s", rUtilities::rNormalize(username));
    strcpy(currentIM, buffer);
    if (rSendFlap(PTYPE_DATA, currentIM) == -1)
    {
        if (errno != EWOULDBLOCK)
        {
            close(sock);
            if (errorFunc) (*errorFunc)(0xFF, ErrorText[ERROR_TEXT_LOSTCONNECTION]);
        }
    }
}

void rTOC2::rSetAway(const char *awayMsg) 
{
    char username[BUDDYLEN];
    isSpecialAway = false;
    strcpy(username, user); 
    if (awayMsg == NULL)
    {
	isAway = false;
        LOG("Unsetting away message\r\n");
        sprintf(buffer, "toc_set_away \"\"");
    }
    else if (awayMsg[0] == '\0')
    {
	isAway = false;
        LOG("Unsetting away message\r\n");
        sprintf(buffer, "toc_set_away \"\"");
    }
    else
    {
        isAway = true;
        strcpy(currentIM, awayMsg);
	strcpy(awayMessage, currentIM);
        sprintf(buffer, "toc_set_away \"%s\"", rEncode(currentIM));
        LOG("Setting away message: %s\r\n", buffer);
    }
    strcpy(currentIM, buffer);
    rSendFlap(PTYPE_DATA, currentIM);
    LOG("Sent away message flap\r\n");
}

void rTOC2::rSetAwayMessage(const char *awayMsg)
{
    if (awayMsg == NULL || awayMsg[0] == '\0')
    {
        awayMessage[0] = '\0';
        if (isAway || isSpecialAway)
            rSetAway(NULL);
        return;
    }

    strncpy(awayMessage, awayMsg, sizeof(awayMessage) - 1);
    awayMessage[sizeof(awayMessage) - 1] = '\0';
    if (isAway || isSpecialAway)
        rSetAway(awayMsg);
}

char *rTOC2::rEncode(char* buf)
{
    int i, j;
    for (i=0; i<(int)strlen(buf); i++)
    {
       switch(buf[i])
       {
           case '\r': 
           {
               buf[i] = '<';
               for (j=strlen(buf); j>i; j--)
               {
                   buf[j+3] = buf[j];
               }
               buf[i+1] = 'b';
               buf[i+2] = 'r';
               buf[i+3] = '>';
               i+=3;
               break;
           }
           case '{':
           case '}':
           case '\\':
           case '"':
           {
               for (j=strlen(buf); j>i-1; j--)
               {
                   buf[j+1] = buf[j];
               }
               buf[i+1] = '\\';
               i++;
               break;
           }
       }
   }
   return buf;
}

void rTOC2::rGetInfo(const char *user)
{
    char username[BUDDYLEN];
    strcpy(username, user);
    sprintf(buffer, "toc_get_info %s", username);
    strcpy(currentIM, buffer);
    rSendFlap(PTYPE_DATA, currentIM);
}

int rTOC2::rSimpleReadInfo(const char *url)
{
    struct sockaddr_in infoServer;

    char parseUrl[256];
    char site[256];
    char theUrl[256];
    char *pch;
    int  thePort;
    if (httpSock != 0)
        close (httpSock);
    httpSock = 0;
    WPRINT("Parsing url: %s", url);
    memset(buffer, 0, BUFLEN-1);
    strcpy(parseUrl, url);
    strcpy(theUrl, url);
    if (strncasecmp(parseUrl, "http://", 7) == 0)
    {
         strcpy(theUrl, &url[7]);
         strcpy(parseUrl, &url[7]);
    }
    pch = strtok(parseUrl, ":");
    if (pch == NULL)
    {
        return ERROR_PARSE_FAIL;
    }
    strcpy(theUrl, pch);
    pch = strtok(NULL, "/");
    if (pch == NULL)
    {
        return ERROR_PARSE_FAIL;
    }
    thePort = atoi(pch);
    pch = strtok(NULL, "\0");
    if (pch == NULL)
        return ERROR_PARSE_FAIL;
    strcpy(site, pch);
    //parse complete
        
    //create the socket
    WPRINT("resolving host %s", theUrl);
    struct hostent *host;
    if ((host = gethostbyname(theUrl)) == NULL)
    {
    	if (errorFunc) (*errorFunc)(0xFF, ErrorText[ERROR_TEXT_PROFILE_HOST]);
        return ERROR_RESOLVE_IP;
    }
    WPRINT("creating socket");
    if ((httpSock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
    {
    	if (errorFunc) (*errorFunc)(0xFF, ErrorText[ERROR_TEXT_PROFILE_SOCKET]);
        httpSock = 0;
        return ERROR_SOCKET_CREATE;
    }

    memset(&infoServer, 0, sizeof(infoServer));
    infoServer.sin_family = AF_INET;
    infoServer.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);
    infoServer.sin_port = htons(thePort);
     
    WPRINT("connecting to %lu on port %d", infoServer.sin_addr.s_addr, thePort);
    if (connect(httpSock, (struct sockaddr*) &infoServer, sizeof(infoServer)) < 0)
    {
        httpRetries++;
        if (httpRetries > HTTPRETRY)
        {
    	    if (errorFunc) (*errorFunc)(0xFF, ErrorText[ERROR_TEXT_PROFILE_CONNECT]);
            httpSock = 0;
            return ERROR_CONNECT_FAIL;
        }
        return ERROR_RETRY;
    }
    memset(httpInfo, 0, BUFLEN);
    httpSize = 0;

    sprintf(buffer, "GET /%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", site, theUrl);
    WPRINT("issuing %s", buffer);
    int size = 0;
    WPRINT("sending get string!");
    size = send(httpSock, buffer, strlen(buffer), 0); //see if we can get the info
    if (size <= 0)
    {
        httpRetries++;
        if (httpRetries > HTTPRETRY)
        {
    	    if (errorFunc) (*errorFunc)(0xFF, ErrorText[ERROR_TEXT_PROFILE_REQUEST]);
            close(httpSock);
            httpSock = 0;
            return ERROR_SEND_FAIL;
        }
        return ERROR_RETRY;
    }
    WPRINT("done");
    return ERROR_NO_ERROR;
}

bool rTOC2::rPollProfile()
{
    int recvBytes = 0;
    if (!waitForReadable(httpSock, 0))
        return false;
    WPRINT("going to recv\n\n");
    recvBytes = recv(httpSock, &httpInfo[httpSize], BUFLEN-httpSize-1, 0);
    if (recvBytes < 0)
    {
        if (errno != EWOULDBLOCK)
        {
            WPRINT("discon 1");
            close(httpSock);
            rRetryProfile(ErrorText[ERROR_TEXT_PROFILE_RESPOND]);
            return false;
        }
    }
    else if (recvBytes == 0)
    {
        WPRINT("profile stream closed");
        if (httpSize > 0)
        {
            char *retData;
            if (!rCheckProfile())
                return false;
            retData = rFormatProfile();
            close(httpSock);
            httpSock = 0;
            if (getinfoFunc) (*getinfoFunc)(retData);
            return true;
        }
        close(httpSock);
        rRetryProfile(ErrorText[ERROR_TEXT_PROFILE_DISCONNECT]);
        return false;
    }
    else
    {
        httpSize+=recvBytes;
        WPRINT("got... %s\n\n", &httpInfo[httpSize]);
        if ( strstr(httpInfo, "</HTML>") )
        {
            char *retData;
            if (!rCheckProfile())
                return false;
            retData = rFormatProfile();
            close(httpSock);
            httpSock = 0;
            if (getinfoFunc) (*getinfoFunc)(retData);
            return true;
        }
    }
    return false;
}

char* rTOC2::rFormatProfile()
{
    char *startLoc = NULL;
    char *endLoc;
    char *tmpPos;
    WPRINT("removing carrage returns");
    while (1)
    {
        tmpPos = strstr(httpInfo, "\r\n");
        if (tmpPos == NULL)
            break;
        while (*tmpPos != '\0')
        {
            *tmpPos = *(tmpPos+1);
            tmpPos++;
        }
        *tmpPos = '\0';
    }
    while (1)
    {
        tmpPos = strstr(httpInfo, "\r");
        if (tmpPos == NULL)
            break;
        *tmpPos = '\n';
    }

    startLoc = strstr(httpInfo, "</TITLE>");
    if (startLoc != NULL)
    {
        startLoc += strlen("</TITLE>");
    }
    else
    {
        startLoc = strstr(httpInfo, "<BODY>");
        if (startLoc != NULL)
            startLoc += strlen("<BODY>");
        else
        {
            startLoc = strstr(httpInfo, "\n\n");
            if (startLoc != NULL)
                startLoc += 2;
            else
                startLoc = httpInfo;
        }
    }

    endLoc = &httpInfo[strlen(httpInfo)-1];
    char *legend = strstr(startLoc, "Legend:");
    if (legend != NULL)
    {
        *legend = '\0';
    }
    else
    {
        char *htmlEnd = strstr(startLoc, "</HTML>");
        if (htmlEnd != NULL)
            *htmlEnd = '\0';
    }
    WPRINT("returning %x", startLoc);
    
    return startLoc;
}
    
    
bool rTOC2::rCheckProfile()
{
    char *startLoc;
    char sLen[15];
    int ct;
    int byteLen = 0;
    WPRINT("found string in info! checking if valid");

    startLoc = strstr(httpInfo, "Content-Length:");
    if (startLoc == NULL)
    {
        startLoc = strstr(httpInfo, "<HTML>");
        if (startLoc == NULL)
            startLoc = strstr(httpInfo, "<html>");
        if (startLoc == NULL)
        {
            WPRINT("Could not find content-length or HTML");
            close(httpSock);
            rRetryProfile(ErrorText[ERROR_TEXT_PROFILE_CORRUPT]);
            return false;
        }
        return true;
    }
    //read the bytes
    startLoc+=strlen("Content-Length: ");
    ct = 0;
    while (startLoc[ct] != '\n')
    {
        sLen[ct] = startLoc[ct];
        ct++;
    }
    sLen[ct] = '\0';
    byteLen = atoi(sLen);
    WPRINT("length is %d", byteLen);
    //find the strlen of the profile
    startLoc = strstr(httpInfo, "<HTML>");
    if (startLoc == NULL)
    {
        startLoc = strstr(httpInfo, "<html>");
        if (startLoc == NULL)
        {
            WPRINT("could not find <HTML>");
            close(httpSock);
            rRetryProfile(ErrorText[ERROR_TEXT_PROFILE_CORRUPT]);
            return false;
        }
    }
    if ( (int)(strlen(startLoc)+1) != byteLen)
    {
        WPRINT("size mismatch %d to %d", strlen(startLoc), byteLen);
        close(httpSock);
        rRetryProfile(ErrorText[ERROR_TEXT_PROFILE_CORRUPT]);
        return false;
    }
    return true;
}


bool rTOC2::rRetryProfile(char *errorString)
{
    httpSock = 0;
    httpRetries++;
    if (httpRetries > HTTPRETRY)
    {
        if (errorFunc) (*errorFunc)(0xFF, errorString);
        return false;
    }
    while (1)
    {
        if (rSimpleReadInfo(httpURL) != ERROR_RETRY)
        break;
    }
    return true;
}

char* rTOC2::rGetFirstBuddyGroup(int &online, int &total, int &current)
{
   rBuddyGroup *gp;
   char *name;
   currentBuddyGroup = 0;
   currentBuddy = 0;
   gp = buddyList->getGroup(0);
   if (gp == NULL)
       return NULL;
   name = gp->getName();
   current = currentBuddyGroup;
   online = gp->getOnlineBuddies();
   total = gp->getTotalBuddies();
   return name;
}

char* rTOC2::rGetNextBuddyGroup(int &online, int &total, int &current)
{
   rBuddyGroup *gp;
   char *name;
   currentBuddy = 0;
   current = ++currentBuddyGroup;
   gp = buddyList->getGroup(currentBuddyGroup);
   if (gp == NULL)
       return NULL;
   name = gp->getName();
   online = gp->getOnlineBuddies();
   total = gp->getTotalBuddies();
   return name;
}

char* rTOC2::rGetFirstBuddy(int group, int &stat)
{
   char *name;
   rBuddyGroup *gp;
   currentBuddy = 0;
   gp = buddyList->getGroup(group);
   if (gp == NULL)
       return NULL;
   
   currentBuddyGroup = group;
   name = gp->getBuddyName(0);
   if (name == NULL) 
   {
       currentBuddy = 0;
       return NULL;
   }
   stat = gp->getBuddyStat(name);
   return name;
}

char* rTOC2::rGetNextBuddy(int &stat)
{
   rBuddyGroup *gp;
   char *name;
   currentBuddy++;
   gp = buddyList->getGroup(currentBuddyGroup);
   if (gp == NULL)
       return NULL;
   
   name = gp->getBuddyName(currentBuddy);
   if (name == NULL) 
   {
       currentBuddy = 0;
       return NULL;
   }
   stat = gp->getBuddyStat(name);
   return name;
}

void rTOC2::rDisconnect()
{
    WPRINT("Disconnection");
    isDisconnected = true;
    for (int i=0; i<10; i++)
        send(sock, "DISCONNECT ME!", 14, 0);
    send(sock, "\0", 1, 0);
    send(sock, " ", 1, 0);
    close(sock);
    sock = 0;	
}

void rTOC2::rCreateTimer()
{
    irqSet(IRQ_TIMER0, rTimerTimeout);
    TIMER0_CR = TIMER_ENABLE | TIMER_DIV_256 | 64;
    TIMER0_DATA = (u16)TIMER_FREQ_256(1);//30 is a good number!!
    irqEnable(IRQ_TIMER0);
}

void rTOC2::rStopTimer()
{
    if (timerStatus == TIMER_RUNNING_CHECK)
        timerStatus = TIMER_STOPPED_CHECK;
    else if (timerStatus == TIMER_RUNNING_WAIT)
        timerStatus = TIMER_STOPPED_WAIT;
}

void rTOC2::rStartTimer(bool reset)
{
    if (reset)
    {
        timerTick = 0;
        timerStatus = TIMER_RUNNING_CHECK;
        timerState = TIMER_STATE_NORMAL;
    }
    else
    {
        if (timerStatus == TIMER_STOPPED_CHECK)
            timerStatus = TIMER_RUNNING_CHECK;
        else if (timerStatus == TIMER_STOPPED_WAIT)
            timerStatus = TIMER_RUNNING_WAIT;
    }
}
        
void rTOC2::rDestroyTimer()
{
    irqSet(IRQ_TIMER0, 0);
    TIMER0_CR = 0;
}

void rTOC2::rTimerTimeout()
{
    if (tocInstance->timerStatus == TIMER_RUNNING_CHECK)
    {
        tocInstance->timerTick++;
        if (tocInstance->timerTick >= TIMEOUT)
        {
            //WPRINT("Check timed out");	
            tocInstance->timerState = TIMER_STATE_PING;
            tocInstance->timerStatus = TIMER_STOPPED_CHECK;
            tocInstance->timerTick = 0;
        }
    }
    else if (tocInstance->timerStatus == TIMER_RUNNING_WAIT)
    {
        tocInstance->timerTick++;
        if (tocInstance->timerState == TIMER_STATE_GOTACK) //we got the response, we are still connected!
        {
            //WPRINT("Got an ack!");	
            tocInstance->timerStatus = TIMER_RUNNING_CHECK;
            tocInstance->timerState = TIMER_STATE_NORMAL;
        }
        else if (tocInstance->timerTick >= TIMEOUT) //we are probably disconnected
        {
            //WPRINT("we timed out, disconnected?");	
            tocInstance->timerState = TIMER_STATE_TIMEOUT;
            tocInstance->timerStatus = TIMER_STOPPED_WAIT;
            tocInstance->timerTick = 0;
            tocInstance->isDisconnected = true;
        }
    }
}
        
void rTOC2::rSetSpecialAway(char *msg)
{
    if (!msg)
    {
        if (isSpecialAway)
        {
            rSetAway("");
            isSpecialAway = false;
        }
    }
    else
    {
        if (msg[0] == '\0')
        {
            if (isSpecialAway)
            {
                rSetAway("");
                isSpecialAway = false;
            }
        }
        else
        {
            if (!isSpecialAway && !isAway)
            {
                rSetAway(msg);
                isSpecialAway = true;
            }
        }
    }
}
    	
    
            
            

void rTOC2::debugBuddy()
{
/*    char *testString = "g:Buddies\nb:Ryan\nb:Timmy\ng:Friends\nb:Valerie\nb:Zed\ng:Coolies\nb:Jim\nb:Ryan\0";
    char *sGroup;
    int nGroupID = 0;
    buddyList = new rBuddyList();
    buddyList->createList(testString); 
    char *s1 = "Ryan:T:0:1:0:000\0"; 
    rParseUpdateMessage(s1);
    
    sGroup = buddyList->getBuddyGroups(sourceUser, &nGroupID);
    if (updateFunc) (*updateFunc)(sGroup, buddyList->getGroupNumber(sGroup), sourceUser, sourceStat);
    while ( (sGroup = buddyList->getBuddyGroups(NULL, &nGroupID)) != NULL)
    {
        if (updateFunc) (*updateFunc)(sGroup, buddyList->getGroupNumber(sGroup), sourceUser, sourceStat);
    }
        
    char *s2 = "Zedi:T:0:1:0:000\0"; 
    rParseUpdateMessage(s2);
    sGroup = buddyList->getBuddyGroups(sourceUser, &nGroupID);
    if (updateFunc) (*updateFunc)(sGroup, buddyList->getGroupNumber(sGroup), sourceUser, sourceStat);
    char *s3 = "Jim:T:0:1:0:000\0"; 
    rParseUpdateMessage(s3);
    sGroup = buddyList->getBuddyGroups(sourceUser, &nGroupID);
    if (updateFunc) (*updateFunc)(sGroup, buddyList->getGroupNumber(sGroup), sourceUser, sourceStat);*/
}
