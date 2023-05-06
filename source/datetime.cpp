#include <printf.h>
#include <string.h>

#include "datetime.h"

void getMonthStr(char* dest, int month, bool shortened)
{
    // month adjusted for the fact that table contains
    // two entries per month and is zero-indexed
    int _monthAdj = (month * 2) - 1;

    char* p = MONTH_NAMES_ARR;    // char iterator
    char* name = MONTH_NAMES_ARR; // current name
    int seps = 0;                 // number of entries we've passed
    int len = 0;                  // number of chars between previous sep and now

    for (; *p; p++, len++)
    {
        if (*p == '|')
        {
            // if we want shortened name, subtract one
            if (_monthAdj - (shortened ? 1 : 0) == seps)
            {
                strncpy(dest, name, len);
                dest[len - 1] = '\0';
            }

            len = 0;      // if not our month, reset len
            name = p + 1; // set name to next entry
            seps++;
        }
    }
}
void dosTimeToS(char* dest, u16 time)
{
    int hour, min, sec;
    hour = time >> 11;
    min = (time & 0x7E0) >> 5;
    sec = (time & 0x1F) * 2;
    sprintf(dest, "%02d:%02d", hour, min);
}
void dosDateToS(char* dest, u16 date)
{
    char tmp[0x10];
    int year, month, day;
    year = 1980 + (date >> 9);
    month = (date & 0x1E0) >> 5;
    day = date & 0x1F;

    getMonthStr(tmp, month, true);
    sprintf(dest, "%s %d", tmp, day);
}