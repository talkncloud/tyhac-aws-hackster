/*
* Description:  This handles all aspects of the RGB light strips either side of the unit.
*               
*               Note: I think the lights are only on the AWS M5Stack Core2?
*
*               Bug: There is a current bug where the RGB is out of order, it is currently GRB. I have
*                    corrected this in the setRgbColor function but this may be resolved one day in
*                    fastled and reverted in this program. See link below.
*
*                    Link: https://github.com/FastLED/FastLED/issues/827#issuecomment-505668673
*               
* Author:       Mick Jacobsson - (https://www.talkncloud.com)
*/
#include "tyhac_rgb.h"

CRGB ledsBuff[LEDS_NUM];

const char *orange = "orange";
const char *orangeAlt = "orangeAlt";
const char *red = "red";
const char *blue = "blue";
const char *green = "green";

/*
*   setRgbColor(red, green, blue)
*   Each led is set individually, the function will set the color based on the values passed
*   to the function. The noteable setting, delay will adjust how quickly you want each led
*   to come on. Adjusting this can make some cool effects.
*   
*
*   Source/Credit: https://github.com/m5stack/M5Core2/blob/d7824df53fe77fd19e17f414e4215a6a61814eb2/examples/core2_for_aws/FactoryTest/FactoryTest.ino
*/
void setRgbColor(uint8_t green, uint8_t blue, uint8_t red)
{
    for (int i = 0; i < LEDS_NUM; i++)
    {
        ledsBuff[i] = ledsBuff[i].setRGB(green, red, blue);
        FastLED.show();
        delay(100);
        FastLED.setBrightness(100); // turn down the brightness, blinding at night
    }
}

/*
*   changeRgbColor("orange")
*   Just a simple if/else to change the color of the RGB strips based on the char value passed
*   to the function.
*
*   Note: Because of the mentioned above setting color using the FastLed enum's e.g. 
*         CRGB::SomeColor will result in the wrong colors. This is why I've written 
*         this funciton.
* 
*/
void changeRgbColor(const char *changeColor)
{

    if (changeColor == orange)
    {
        uint8_t red = 252;
        uint8_t green = 186;
        uint8_t blue = 3;

        setRgbColor(green, blue, red);
    }
    else if (changeColor == orangeAlt)
    {
        uint8_t red = 255;
        uint8_t green = 153;
        uint8_t blue = 0;

        setRgbColor(green, blue, red);
    }
    else if (changeColor == green)
    {
        uint8_t red = 0;
        uint8_t green = 201;
        uint8_t blue = 0;

        setRgbColor(green, blue, red);
    }
    // Turning red to 0, 0, 255 did not work?
    else if (changeColor == red)
    {
        uint8_t red = 201;
        uint8_t green = 0;
        uint8_t blue = 0;

        setRgbColor(green, blue, red);
    }
    else if (changeColor == blue)
    {
        uint8_t green = 0;
        uint8_t blue = 201;
        uint8_t red = 0;

        setRgbColor(green, blue, red);
    }
    else
    {
        uint8_t green = 3;
        uint8_t blue = 3;
        uint8_t red = 3;

        setRgbColor(green, blue, red);
    }
}

/*
*   setupRgb()
*   Initialise the RGB lights.
*   The default setting is a dim light color.
*
*/
void setupRgb()
{
    FastLED.addLeds<SK6812, LEDS_PIN>(ledsBuff, LEDS_NUM);

    for (int i = 0; i < LEDS_NUM; i++)
    {
        ledsBuff[i] = ledsBuff[i].setRGB(102, 102, 102);
    }
    FastLED.show();
}