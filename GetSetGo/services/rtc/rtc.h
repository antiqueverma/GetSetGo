
#ifndef RTC_H_
#define RTC_H_

#include <stdint.h>

typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} rtc_time_t;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
} rtc_date_t;

typedef struct {
    rtc_time_t time;
    rtc_date_t date;
} rtc_datetime_t;

void RTC_Init(void);
void RTC_SetTime(rtc_time_t time);
void RTC_SetDate(rtc_date_t date);
rtc_datetime_t RTC_GetDateTime(void);
void RTC_getFormattedTime(char *buffer);
void RTC_getFormattedDate(char *buffer);
void RTC_getFormattedDateTime(char *buffer);
void RTC_IncrementSeconds(void);

#endif /* RTC_H_ */