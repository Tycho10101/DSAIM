#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include "buddy.h"
#include "utils.h"

#include "wifidebug.h"

#include "mem.h"

/********************************************Single buddy instance*******************************************/
rBuddy::rBuddy(char *name, int val)
{
    userName = new char[strlen(name)+1];
    strcpy(userName, name);
    stat = val;
    alias = NULL;
}

rBuddy::rBuddy(char *name, char *al, int val)
{
    userName = new char[strlen(name)+1];
    strcpy(userName, name);
    stat = val;
    alias = new char[strlen(al)+1];
    strcpy(alias, al);
}
rBuddy::rBuddy() {};

rBuddy::~rBuddy() 
{
    if (userName != NULL)
        delete [] userName;
    if (alias != NULL)
        delete [] alias;
}

int rBuddy::setStat(int val)
{
    int result = 0;
    if ( (stat == BUDDY_UNAVAILABLE) && ((val & BUDDY_AVAILABLE) == BUDDY_AVAILABLE))
    {
        result = 1;
    }
    else if ( ( (stat & BUDDY_AVAILABLE) == BUDDY_AVAILABLE) && (val == BUDDY_UNAVAILABLE) )
    {
        result = -1;
    }
    stat = val;
    return result;
}

/************************************************************************************************************/



/********************************************Single group instance*******************************************/
rBuddyGroup::rBuddyGroup() {}
rBuddyGroup::rBuddyGroup(char *name)
{
    strcpy(groupName, name);
    size = 4;  //4 initial max buddies in each group
    total = onlineTotal = 0;
    Buddies = new rBuddy*[size];
}

rBuddyGroup::~rBuddyGroup()
{
    for (unsigned int i=0; i<total; i++)
        delete Buddies[i];  //get rid of each individual buddy
    delete [] Buddies; //Get rid of the pointer to array of buddies;
}

void rBuddyGroup::addBuddy(rBuddy *buddy) //Buddy is already created
{
    unsigned int i;
    rBuddy **tmpBuddies;
    char *userName = buddy->getName(); int path;
    if (total+1 >= size) //we are out of buffer space
    {
        if ( userName[0] == 'O' || userName[0] == 'o')
        {
            path = 0;
        }
        size*=2;
        tmpBuddies = new rBuddy*[size];
        for (i=0; i<total; i++)
            tmpBuddies[i] = Buddies[i];
        tmpBuddies[i] = buddy; //added new buddy
        delete [] Buddies;  //get rid of the old pointer
        Buddies = tmpBuddies;  //assign its new location
    }
    else
    {
        Buddies[total] = buddy;
    }
    total++;
}
            
void rBuddyGroup::addBuddy(char *buddyName, char *al, int bStat) //Buddy is not created
{
    rBuddy *buddy;
    if (al == NULL)
        buddy = new rBuddy(buddyName, bStat);
    else
        buddy = new rBuddy(buddyName, al, bStat);
    addBuddy(buddy);
} 
     
void rBuddyGroup::removeBuddy(rBuddy *buddy)
{
    unsigned short add = 0;
    for (unsigned int i=0; i<total; i++)
    {
        if (Buddies[i] == buddy) //found it
        {
            delete Buddies[i]; //get rid of it
            add = 1;
            total--;
            if (i == total) break;
        }
        Buddies[i] = Buddies[i+add];
    }
}
        
void rBuddyGroup::removeBuddy(char *buddyName)
{
    for (unsigned int i=0; i<total; i++)
    {
        if (rUtilities::rNormalizeCompare(buddyName, Buddies[i]->getName()) == 0)
        {
            removeBuddy(Buddies[i]);
            break;
        }
    }
}
        
void rBuddyGroup::sortBuddies()
{
    rBuddy *tmpBuddy;
    for (unsigned int i=0; i<total-1; i++)
    {
        for(unsigned int j=i+1; j<total; j++)
        {
            if (rUtilities::rNormalizeCompare(Buddies[i]->getName(), Buddies[j]->getName()) > 0) //swap 'em
            {
                tmpBuddy = Buddies[j];
                Buddies[j] = Buddies[i];
                Buddies[i] = tmpBuddy;
            }
        }
    }
}

void rBuddyGroup::setBuddyStat(char *bName, int bStat)
{
    for (unsigned int i=0; i<total; i++)
    {
        if (rUtilities::rNormalizeCompare(Buddies[i]->getName(), bName) == 0) //found the buddy
        {
            onlineTotal +=Buddies[i]->setStat(bStat); 
        }
    }
}    
    
int rBuddyGroup::getBuddyStat(char *bName)
{
    for (unsigned int i=0; i<total; i++)
    {
        if (rUtilities::rNormalizeCompare(Buddies[i]->getName(), bName) == 0) //found the buddy
            return Buddies[i]->getStat();
    }
    return -1;
}    

char *rBuddyGroup::getBuddyName(unsigned int bNum)
{
    if (bNum >= total)
        return NULL;
    return Buddies[bNum]->getName();
}

char *rBuddyGroup::getBuddyAlias(unsigned int bNum)
{
    if (bNum >= total)
        return NULL;
    return Buddies[bNum]->getAlias();
}

/************************************************************************************************************/

rBuddyList::rBuddyList()
{
    size = 4;  //default to 4 group max
    totalGroups = 0;
    Groups = new rBuddyGroup*[size];
    tmpName = NULL;
}

rBuddyList::~rBuddyList()
{
    for (unsigned int i=0; i<totalGroups; i++)
        delete Groups[i];  //get rid of each individual group
    delete [] Groups; //Get rid of the pointer to array of buddies;
}

unsigned int rBuddyList::getTotalBuddies()
{
    unsigned int total = 0;
    for (unsigned int i=0; i<totalGroups; i++)
        total+=Groups[i]->getTotalBuddies();
    return total;
}

unsigned int rBuddyList::getOnlineBuddies()
{
    unsigned int total = 0;
    for (unsigned int i=0; i<totalGroups; i++)
        total+=Groups[i]->getOnlineBuddies();
    return total;
}

void rBuddyList::addGroup(rBuddyGroup *group)
{
    unsigned int i;
    rBuddyGroup **tmpGroups;
    if (totalGroups+1 >= size) //we are out of buffer space
    {
        size*=2;
        tmpGroups = new rBuddyGroup*[size];
        for (i=0; i<totalGroups; i++)
            tmpGroups[i] = Groups[i];
        tmpGroups[i] = group; //added new group
        delete [] Groups;  //get rid of the old pointer
        Groups = tmpGroups;  //assign its new location
    }
    else
        Groups[totalGroups] = group;
    totalGroups++;
}

void rBuddyList::addGroup(char *name)
{
    rBuddyGroup *n = new rBuddyGroup(name);
    addGroup(n);
}

void rBuddyList::removeGroup(rBuddyGroup *group)
{
    unsigned short add = 0;
    for (unsigned int i=0; i<totalGroups; i++)
    {
        if (Groups[i] == group) //found it
        {
            delete Groups[i]; //get rid of it
            add = 1;
            totalGroups--;
            if (i == totalGroups) break;
        }
        Groups[i] = Groups[i+add];
    }
}

void rBuddyList::removeGroup(char *groupName)
{
    for (unsigned int i=0; i<totalGroups; i++)
    {
        if (strcasecmp(groupName, Groups[i]->getName()) == 0)
        {
            removeGroup(Groups[i]);
            break;
        }
    }
}

void rBuddyList::sortGroups()
{
    rBuddyGroup *tmpGroup;
    for (unsigned int i=0; i<totalGroups-1; i++)
    {
        for(unsigned int j=i+1; j<totalGroups; j++)
        {
            if (rUtilities::rNormalizeCompare(Groups[i]->getName(), Groups[j]->getName()) > 0) //swap 'em
            {
                tmpGroup = Groups[j];
                Groups[j] = Groups[i];
                Groups[i] = tmpGroup;
            }
        }
    }
}

void rBuddyList::createList(char *buffer)
{
    int i = 0;
    int j = 0;
    char name[100];
    char alias[100];
    rBuddyGroup *currentGroup = NULL;
    rBuddyGroup *n;
    unsigned int len = strlen(buffer);
    
    while ((unsigned int) i < len)
    {
        if ( (buffer[i] == 'g') && (buffer[i+1] == ':') ) //new grouping
        {
            i+=2;
            j = 0;
            while ( (buffer[i] != ':') && (buffer[i] != 10) && (buffer[i] != '\0'))
            {
                name[j] = buffer[i];
                i++; j++;
            }
            name[j] = '\0';
            if (buffer[i] == ':') //alias follows, do nothing for now
            {
                while (buffer[i] != 10) i++;
            }
            if (buffer[i] != '\0') //end of buffer           
               i++;  //next line
            
            // create the group
            n = new rBuddyGroup(name);
            addGroup(n);
            currentGroup = n;
        }
        else if ( (buffer[i] == 'b') && (buffer[i+1] == ':') ) //new buddy
        {
            if (currentGroup == NULL) //this buddy has no group
            {
                n = new rBuddyGroup("rDefault");
                addGroup(n);
                currentGroup = n;
            }
            i+=2;
            j = 0;
            while ( (buffer[i] != ':') && (buffer[i] != 10) && (buffer[i] != '\0'))
            {
                name[j] = buffer[i];
                i++; j++;
            }
            name[j] = '\0';
            alias[0] = '\0';
            if (buffer[i] == ':') //alias follows, do nothing for now
            {
                i++;
                j=0;
                while ( (buffer[i] != 10) && (buffer[i] != ':') )
                {
                    alias[j] = buffer[i];
                    j++; i++;
                }
		if (buffer[i] == ':')
		{
                    while (buffer[i] != 10)
                        i++;
                }
                alias[j] = '\0';
            }
            if (buffer[i] != '\0') //end of buffer           
               i++;  //next line
            
            // create the buddy
            if (alias[0] == '\0')
            {
                currentGroup->addBuddy(name);
            }
            else
            {
                currentGroup->addBuddy(name, alias, (int)BUDDY_UNAVAILABLE);
            }
        }
        else //unknown element
        {
            while ((buffer[i] != 10) && (buffer[i] != '\0'))
            {
                i++;
            }
            if (buffer[i] == 10)
                i++;
        }
   }
}

void rBuddyList::setBuddyStat(char *bName, int bStat)
{
    for (unsigned int i=0; i<totalGroups; i++)
    {
        Groups[i]->setBuddyStat(bName, bStat);
    }
}    
                 
int rBuddyList::getBuddyStat(char *bName)
{
    int z = 0;
    for (unsigned int i=0; i<totalGroups; i++)
    {
        z = Groups[i]->getBuddyStat(bName);
        if (z != -1)
           return z;
    }
    return -1;
}    
            
char *rBuddyList::getGroupName(unsigned int num)
{
    if (num >= totalGroups)
        return NULL;
    return Groups[num]->getName();
}

char *rBuddyList::getBuddyName(unsigned int gNum, unsigned int bNum)
{
    if (gNum >= totalGroups)
        return NULL;
    return Groups[gNum]->getBuddyName(bNum);
}

char *rBuddyList::getBuddyAlias(unsigned int gNum, char *name)
{
    if (gNum >= totalGroups)
        return NULL;
    rBuddyGroup *g = Groups[gNum];
    int i;
    for (i=0; i<(int)g->getTotalBuddies(); i++)
    {
        if (!rUtilities::rNormalizeCompare(name, g->getBuddyName(i)))
        {
            return g->getBuddyAlias(i);
        }
    }   
    return NULL;
}

char rBuddyList::getGroupNumber(char *name)
{
    for (int i=0; i<(int)totalGroups; i++)
    {
        if (!rUtilities::rNormalizeCompare(name, getGroupName((unsigned int)i)))
        {
            return (char)i;
        }
    }
    return -1;
}
        
    

rBuddyGroup *rBuddyList::getGroup(unsigned int gp)
{
    if (gp >= totalGroups)
        return NULL;
    return Groups[gp];
}

char *rBuddyList::getBuddyGroups(char *name, int *id)
{
    rBuddyGroup *s;
    char *buddyName;
    int i,j;
    *id = -1;
    nTmpBuddy = 0;

    if (name != NULL)
    {
        nTmpGroup = 0;
        nTmpBuddy = 0;
        tmpName = name;
    }
    if (tmpName == NULL)
        return "tmpName NULL";
    for (i=nTmpGroup; i<(int)totalGroups; i++)
    {
        s = getGroup((unsigned int)i);
        for (j=nTmpBuddy; j<(int)s->getTotalBuddies(); j++)
        {
            buddyName = s->getBuddyName(j);
            if (rUtilities::rNormalizeCompare(tmpName, buddyName) == 0) //found a group
            {
                nTmpGroup = i+1;
                *id = i;
                return s->getName();
            }
        }
    }
    return NULL;
}
