#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "nds.h"
#include "clock.h"
#include <../lib/libfb/libcommon.h>

rClock::rClock()
{
    drawX = drawY = 1;
    min[0] = min[1] = -1;
    color = RGB15(0,0,0);
    update = true;
    clockString[0] = '\0';
}

rClock::~rClock() {}

int rClock::rNeedsDrawn()
{
    time_t unixTime;
    struct tm *timeStruct;

    if (update)
    {
        update = false;
        return 2;
    }

    unixTime = time(NULL);
    timeStruct = gmtime((const time_t *)&unixTime);
    if (!timeStruct)
        return 0;

    min[1] = min[0];
    min[0] = timeStruct->tm_min;
    if (min[0] != min[1])
        return 2;
    return 0;
}

int rClock::rDraw()
{
    if (font == NULL)
        return 0;
    time_t unixTime = time(NULL);
    struct tm *timeStruct = gmtime((const time_t *)&unixTime);
    int hour;
    bool isPM;

    if (!timeStruct)
        return 0;

    hour = timeStruct->tm_hour;
    isPM = hour >= 12;
    if (hour == 0)
        hour = 12;
    else if (hour > 12)
        hour -= 12;

    sprintf(clockString, "%d/%d/%d   %d:%.2d %s",
            timeStruct->tm_mon + 1,
            timeStruct->tm_mday,
            timeStruct->tm_year + 1900,
            hour,
            timeStruct->tm_min,
            isPM ? "PM" : "AM");

    setColor(color);
    fb_dispString(drawX, drawY, clockString);
    fb_dispString(drawX+1, drawY, clockString);
    return 2;
}

