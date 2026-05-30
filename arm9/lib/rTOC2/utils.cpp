#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "utils.h"
#include "wifidebug.h"

char *rUtilities::rNormalize(char *name)
{
    int i, j;
    int len = strlen(name);
    memset(normalizedName, 0, 50);
    j = 0;
    for (i=0; i<len; i++)
    {
        if (name[i] != ' ')
            normalizedName[j++] = tolower(name[i]);
    }
    normalizedName[j] = '\0';
    return normalizedName;
}  
int rUtilities::rNormalizeCompare(char *n1, char *n2)
{
    char s1[50];
    char s2[50];
    strcpy(s1, rNormalize(n1));
    strcpy(s2, rNormalize(n2));
    return strcasecmp(s1, s2);
}

bool rUtilities::rParseUsername(char *userName, char *sn, char *alias)
{
    int i = 0;
    int j=0;
    bool hasAlias = false;
    while (userName[i] != '\0')
    {
        if (userName[i] == ':')
        {
            hasAlias = true;
            break;
        }
        sn[i] = userName[i];
        i++;
    }
    sn[i] = '\0';
    if (hasAlias)
    {
        i++;
        while (userName[i] != '\0')
        {
            alias[j] = userName[i];
            j++;
            i++;
        }
        alias[j] = '\0';
    }
    return hasAlias;
}	

char rUtilities::normalizedName[50];
