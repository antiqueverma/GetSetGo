
#include <stdint.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "rtc.h"

static rtc_datetime_t RTC;

void RTC_Init(void)
{
    RTC.time.hours      = 0;
    RTC.time.minutes    = 0;
    RTC.time.seconds    = 0;
    RTC.date.year       = 2000;
    RTC.date.month      = 1;
    RTC.date.day        = 1;
}

void RTC_SetTime(rtc_time_t time)
{
    RTC.time = time;
}

void RTC_SetDate(rtc_date_t date)
{
    RTC.date = date;
}

rtc_datetime_t RTC_GetDateTime(void)
{
    return RTC;
}

// Time format: HH:MM:SS (MS Excel compatible time format)
void RTC_getFormattedTime(char *buffer)
{
    snprintf(buffer, 16, "%02d:%02d:%02d", RTC.time.hours, RTC.time.minutes, RTC.time.seconds);
}

// Date format: DD-MM-YYYY (MS Excel compatible date format)
void RTC_getFormattedDate(char *buffer)
{
    snprintf(buffer, 16, "%02d-%02d-%04d", RTC.date.day, RTC.date.month, RTC.date.year);
}

// DateTime format: DD-MM-YYYY HH:MM:SS (MS Excel compatible date-time format, comma separated)
void RTC_getFormattedDateTime(char *buffer)
{
    snprintf(buffer, 32, "%02d-%02d-%04d,%02d:%02d:%02d", RTC.date.day, RTC.date.month, RTC.date.year, RTC.time.hours, RTC.time.minutes, RTC.time.seconds);
}

void RTC_IncrementSeconds(void)
{
    RTC.time.seconds++;
    if (RTC.time.seconds >= 60)
    {
        RTC.time.seconds = 0;
        RTC.time.minutes++;
        if (RTC.time.minutes >= 60)
        {
            RTC.time.minutes = 0;
            RTC.time.hours++;
            if (RTC.time.hours >= 24)
            {
                RTC.time.hours = 0;
                RTC.date.day++;
                // Update month taking 31 30 28 29 days accurately
                if ((RTC.date.month == 4 || RTC.date.month == 6 || RTC.date.month == 9 || RTC.date.month == 11) && RTC.date.day > 30)
                {
                    RTC.date.day = 1;
                    RTC.date.month++;
                }
                else if (RTC.date.month == 2)
                {
                    int isLeapYear = (RTC.date.year % 4 == 0 && (RTC.date.year % 100 != 0 || RTC.date.year % 400 == 0));
                    if ((isLeapYear && RTC.date.day > 29) || (!isLeapYear && RTC.date.day > 28))
                    {
                        RTC.date.day = 1;
                        RTC.date.month++;
                    }
                }
                else if (RTC.date.day > 31)
                {
                    RTC.date.day = 1;
                    RTC.date.month++;
                }
                if (RTC.date.month > 12)
                {
                    RTC.date.month = 1;
                    RTC.date.year++;
                }
            }
        }
    }
}


