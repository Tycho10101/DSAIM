#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "nds.h"
#include "debug.h"
#include "globalDefs.h"

char DBGText[255];

void DBGPrint_2(char *s)
{
    asm volatile("mov r0, %0;"
                 "swi 0xff;"
                 : //bi
                 : "r" (s)
                 : "r0");
}
void DBGPrint(char *fmt, ... )
{    
    va_list args;    
    va_start(args, fmt);
    vsnprintf(DBGText, 255, fmt, args);
    va_end(args);
//#define USEDEBUG
#ifdef USEDEBUG
    DBGPrint_2(DBGText);
#endif
    return;
}
