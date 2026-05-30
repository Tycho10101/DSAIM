#ifndef _BUDDY_
#define _BUDDY_

#define BUDDYLEN    30
enum BUDDY_STATUS {
BUDDY_UNAVAILABLE = 0x00,
BUDDY_AVAILABLE   = 0x01,
BUDDY_AWAY        = 0x02,
BUDDY_IDLE        = 0x04,
BUDDY_CELL        = 0x08,
BUDDY_BLOCK       = 0x10,
BUDDY_PERMIT      = 0x20
};

class rBuddy
{
public:
    rBuddy(char*, int);
    rBuddy(char*, char*, int);
    ~rBuddy();
 
    int setStat(int);

    char *getName() { return userName; }
    char *getAlias() { return alias; }
    int getStat() { return stat; }

private:
    rBuddy();
    char *userName;
    char *alias;        
    int stat;
};

class rBuddyGroup
{
public:
    rBuddyGroup(char*);
    ~rBuddyGroup();
    char *getName() { return groupName; }
    unsigned int getTotalBuddies() { return total; }
    unsigned int getOnlineBuddies() { return onlineTotal; }

    void addBuddy(rBuddy *);
    void addBuddy(char *, char *al = NULL, int bStat = (int)BUDDY_UNAVAILABLE);

    void removeBuddy(rBuddy *);
    void removeBuddy(char *);

    void sortBuddies();

    void setBuddyStat(char *, int);
    int getBuddyStat(char *);

    char *getBuddyName(unsigned int);
    char *getBuddyAlias(unsigned int);

private:
    rBuddyGroup();
    char groupName[BUDDYLEN];
    rBuddy **Buddies;
    unsigned size;
    unsigned int total;
    unsigned int onlineTotal;
};

class rBuddyList
{
public:
    rBuddyList();
    ~rBuddyList();

    unsigned int getTotalBuddies();
    unsigned int getOnlineBuddies();

    void addGroup(rBuddyGroup *);
    void addGroup(char *);
    void removeGroup(rBuddyGroup *);
    void removeGroup(char *);

    void setBuddyStat(char *, int);
    int getBuddyStat(char *);

    void sortGroups();

    void createList(char *);

    char *getGroupName(unsigned int);
    char *getBuddyName(unsigned int, unsigned int);
    char *getBuddyAlias(unsigned int, char*);

    rBuddyGroup *getGroup(unsigned int);
    char *getBuddyGroups(char*, int*);
    char getGroupNumber(char *name);
    unsigned int getTotalGroups() {return totalGroups;}

private:
    rBuddyGroup **Groups;
    unsigned int size;
    unsigned int totalGroups;
    char *tmpName;
    int nTmpGroup;
    int nTmpBuddy;
};
    

#endif
