/*
* Description:  Simple program to sync the clock to an NTP server. The other function
*               will simply return the current time in epoch UTC.
*  
* Author:       Mick Jacobsson - (https://www.talkncloud.com)
*/

#include <Arduino.h>

/*
*   setClock()
*   Sync to NTP server, return true or false. 
*
*   Source/Credit: https://lastminuteengineers.com/esp32-ntp-server-date-time-tutorial/
*                  https://randomnerdtutorials.com/epoch-unix-time-esp32-arduino/
*/
bool setClock() {
    // UTC time, adjust timezone if you want
    configTime(0, 0, "pool.ntp.org");

    time_t now;
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo)) {
        Serial.println("TYHAC: NTP sync failed");
        return false;
    } else {
        Serial.println("TYHAC: NTP syncd - " + String(time(&now)));
        return true;
    }

}

/*
*   getTime()
*   Return the current epoch time as a number.
*/
unsigned long getTime() {
    time_t now;
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo)) {
        Serial.println("TYHAC: NTP failed to return time");
        return(0);
    } else {
        // Serial.println("TYHAC: NTP Time - " + String(time(&now)));
        return time(&now);
    }
}