#ifndef _UTILS_
#define _UTILS_

class rUtilities
{
public: 
    static char *rNormalize(char *);
    static int rNormalizeCompare(char *, char *);
    static bool rParseUsername(char*, char *, char *);

private:
    static char normalizedName[50];
};
#endif
