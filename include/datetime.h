#pragma once

extern char MONTH_NAMES_ARR[0x88];
void getMonthStr(char* dest, int month, bool shortened);
void dosTimeToS(char* dest, u16 time);
void dosDateToS(char* dest, u16 date);