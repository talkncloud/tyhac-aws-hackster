/*
* Description:  This handles all aspects of the visual componets on the thing.
*               
*               concepts: screen = LCD / display is whole of screen
*                         screenElem is a sub component of a screen
*                         button is the button elem
*                         buttonAction is the action called from the button
*                         Listerns handled the LCD touch co-ordinate systems
*               
* Author:       Mick Jacobsson - (https://www.talkncloud.com)
*/

#include "tyhac_ui.h"
#include "tyhac_mqtt.h"
#include "tyhac_version.h"
#include "tyhac_rgb.h"

// colors
// I found it easier to use 565 with RGB
uint16_t tyhacBluePrimary = M5.Lcd.color565(0, 106, 219);
uint16_t tyhacBluePrimaryAlt = M5.Lcd.color565(46, 86, 132);
uint16_t tyhacOrangePrimary = M5.Lcd.color565(255, 153, 0);
uint16_t tyhacGreyText = M5.Lcd.color565(666, 666, 666);
uint16_t tyhacRedPrimary = M5.Lcd.color565(219, 0, 0);
uint16_t tyhacGreenPrimary = M5.Lcd.color565(0, 219, 7);

/*
*   buttonLeftAction()
*   called from button click, goes out to desired functions.
*   this activates passive mode (default).
*/
void buttonLeftAction()
{
    Serial.println("TYHAC: Button passive mode");
    M5.Lcd.clearDisplay();
    screenElemHeaderFooter(0, 0);
    messageRequestStats(); // Update dashboard
}

/*
*   buttonMiddleAction()
*   called from button click, goes out to desired functions.
*   currently spare, not used.
*/
void buttonMiddleAction()
{
    // Spare button not used
    Serial.println("TYHAC: Button row middle spare");
}

/*
*   buttonRightAction()
*   called from button click, goes out to desired functions.
*   this activates clinican / field unit mode.
*/
void buttonRightAction()
{
    Serial.println("TYHAC: Button clinician mode");
    M5.Lcd.clearDisplay();
    screenElemHeaderFooter(0, 0);
    screenClinician();
}

/*
*   buttonSubmitAction()
*   clinician mode has two buttons, submit button sends a postive sample
*   to aws for safe keeping.
*/
void buttonSubmitAction()
{
    Serial.println("TYHAC: Submit test button");
    // Record
    tyhacButtonSubmitRequest = 1;
}

/*
*   buttonTestAction()
*   clinician mode test is similar to passive mode, however the 
*   medical professional pushes a button to record.
*/
void buttonTestAction()
{
    Serial.println("TYHAC: Request test button");
    // Record
    tyhacButtonTestRequest = 1;
}

/*
*   buttonVibrate()
*   currently not used, was looking at effects for button push on scren.
*/
void buttonVibrate()
{
    M5.Axp.SetLDOEnable(3, true);
    delay(50);
    M5.Axp.SetLDOEnable(3, false);
}

/*
*   buttonListeners(TouchPoint_t pos)
*   used in the main loop and registers co-orindates when the lcd is touched,
*   based on those co-ordinates we can determine certain actions need
*   to be done. e.g. button.
*/
void buttonListeners(TouchPoint_t pos)
{
    if (pos.y > 240)
    {
        if (pos.x < 109)
        {
            tyhacMode = 0;
            buttonLeftAction();
        }
        if (pos.x < 192 && pos.x > 109)
        {
            // Spare button not used
            buttonMiddleAction();
        }
        if (pos.x < 292 && pos.x > 192)
        {
            tyhacMode = 1;
            buttonRightAction();
        }
    }

    // clinician buttons
    if (pos.y > 110 && pos.y < 150)
    {
        if (pos.x > 50 && pos.x < 150)
        {
            buttonSubmitAction();
        }
        if (pos.x > 150 && pos.x < 250)
        {
            buttonTestAction();
        }
    }
}

/*
*   screenClinician()
*   this screen is used by medical professionals. this will render a screen
*   with elements required for this mode.
*/
void screenClinician()
{
    changeRgbColor("else");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.drawRoundRect(50, 100, 100, 50, 10, tyhacBluePrimary);
    M5.Lcd.fillRoundRect(50, 100, 100, 50, 10, tyhacBluePrimary);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString("submit", 63, 107, 2);

    M5.Lcd.drawRoundRect(170, 100, 100, 50, 10, tyhacOrangePrimary);
    M5.Lcd.fillRoundRect(170, 100, 100, 50, 10, tyhacOrangePrimary);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString("test", 195, 107, 2);
}

/*
*   textSpacing(String text)
*   the dashboard screen shows dynamic ints and strings, the ints can vary in 
*   length which changes how the dash looks. this simply takes the desired
*   int, finds the length and determines the required int positioning 
*   for the Lcd.drawString() method. 
*
*   e.g. 10 samples
*
*   note: this goes to XXXX, anything above will need another if and
*   resulting spacing. more work could be done here to simplify 
*   and determine this with the below pattern.
*   
*/
int textSpacing(String text)
{
    // 1, 10, 100, 1000
    // 1, 2, 3, 4

    if (text.length() == 1)
    {
        return 32;
    }
    if (text.length() == 2)
    {
        return 47;
    }
    if (text.length() == 3)
    {
        return 65;
    }
    if (text.length() == 4)
    {
        return 130;
    }
    return 10; // default, won't work in all cases
}

/*
*   screenDashboard(String samples...)
*   the main dashboard is the home screen on the thing, it shows stats
*   received from an MQTT pub. 
*/
void screenDashboard(String samples, String sampledays, String positive, String negative, String uptime)
{
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString(samples, 10, 30, 2);
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawString("samples", textSpacing(samples), 40, 2); // 30, 30, 2

    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString(sampledays, 10, 70, 2);
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawString("days since sample", textSpacing(sampledays), 80, 2);

    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString(positive, 10, 110, 2);
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawString("positive", textSpacing(positive), 120, 2);

    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString(negative, 10, 150, 2);
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawString("negative", textSpacing(negative), 160, 2);

    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString(uptime, 10, 190, 2);
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawString("days uptime", textSpacing(uptime), 200, 2);
    Serial.println("TYHAC: Screen dashboard rendered");
}

/*
*   screenCovidResult(String status...)
*   this will display the covid result in relation to the result e.g.
*   if negative show text, change color.
*/
void screenCovidResult(String status, String predictClass, String predictPercent, String filename)
{
    uint32_t bgColor = tyhacOrangePrimary;
    uint32_t txtColor = WHITE;
    String predictClassSet = "invalid";
    String predictPercentSet = "0.00";
    if (predictClass == "negative")
    {
        bgColor = DARKGREEN;
        predictClassSet = predictClass;
        predictPercentSet = predictPercent;
        changeRgbColor("green");
    }
    else if (predictClass == "positive")
    {
        bgColor = RED;
        predictClassSet = predictClass;
        predictPercentSet = predictPercent;
        changeRgbColor("red");
    }

    // Now we know its invalid
    if (predictClassSet == "invalid")
    {
        changeRgbColor("orange"); // invalid
    }

    float percentFloat = predictPercentSet.toFloat();
    int percentInt = percentFloat * 100;

    M5.Lcd.fillScreen(bgColor);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.setTextColor(txtColor);
    M5.Lcd.setTextSize(3);
    M5.Lcd.drawString(predictClassSet, 85, 10, 2);
    M5.Lcd.setTextSize(5);
    M5.Lcd.drawString(String(percentInt) + "%", 100, 90, 2);
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawString(filename, 30, 210, 2);
    // If passive mode, show result and return to dashboard
    if (tyhacMode == 0)
    {
        delay(5000);
        M5.Lcd.clearDisplay();
        screenElemHeaderFooter(0, 0);
        messageRequestStats(); // update the dash stats
        changeRgbColor("else");
    }
}

/*
*   screenElemHeaderFooter(int wifi, int aws)
*   think of these elements like a HTML web page, the top and bottom of the lcd
*   screen. this shows the network connectivity status and the version.
*/
void screenElemHeaderFooter(int statusWifi, int statusAws)
{
    // 0 = green, 1 = loading, 2 = failed
    uint16_t wifiColor = tyhacRedPrimary;
    uint16_t awsColor = tyhacRedPrimary;
    delay(100); // for effect
    // change indicators based on status
    if (statusWifi == 0)
    {
        wifiColor = tyhacGreenPrimary;
    }
    else if (statusWifi == 1)
    {
        wifiColor = tyhacOrangePrimary;
    }
    if (statusAws == 0)
    {
        awsColor = tyhacGreenPrimary;
    }
    else if (statusAws == 1)
    {
        awsColor = tyhacOrangePrimary;
    }
    // top bar
    M5.Lcd.setTextColor(tyhacGreyText);
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawString("wifi", 230, 2, 2); // 30, 30, 2
    M5.Lcd.drawCircle(260, 10, 5, wifiColor);
    M5.Lcd.fillCircle(260, 10, 5, wifiColor);
    M5.Lcd.drawString("aws", 270, 2, 2); // 30, 30, 2
    M5.Lcd.drawCircle(300, 10, 5, awsColor);
    M5.Lcd.fillCircle(300, 10, 5, awsColor);
    // bottom bar
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(tyhacGreyText);
    M5.Lcd.drawString(tyhacVersion, 230, 220, 2);
}

/*
*   screenElemLoading(String message)
*   top bar information / status messages, takes a string as the message
*   and displays the message with a blue dot.
*/
void screenElemLoading(String loadingMessage)
{
    uint16_t circleColor;
    if (loadingMessage == "clear")
    {
        circleColor = BLACK;
        loadingMessage = "";
    }
    else
    {
        circleColor = tyhacBluePrimary;
    }
    M5.Lcd.drawRect(25, 1, 200, 17, BLACK); // clear message
    M5.Lcd.fillRect(25, 1, 200, 17, BLACK); // clear message fill
    M5.Lcd.fillCircle(15, 10, 5, circleColor);
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawString(loadingMessage, 25, 2, 2); // 30, 30, 2
}