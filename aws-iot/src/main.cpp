/*
*   Description:  The main program for tyhac.
*                 This program was developed to be used with AWS and the M5 Stack Core2
*                 AWS eduKit device. 
*               
*                 Required: SD card, LM393 Sensor (if using passive mode)
*  
*   Author:       Mick Jacobsson - (https://www.talkncloud.com)
*   Repo:         https://github.com/talkncloud/tyhac-aws-hackster
*/
#include "env.h"
#include <M5Core2.h>
#include <driver/i2s.h>
#include "FS.h"
#include "SPIFFS.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>

// tyhac libs
#include <tyhac_uploads3.h>
#include <tyhac_rgb.h>
#include <tyhac_audio.h>
#include <tyhac_mqtt.h>
#include <tyhac_ntp.h>
#include <tyhac_ui.h>
#include <tyhac_version.h>

/* You only need to format SPIFFS the first time you run a
   test or else use the SPIFFS plugin to create a partition
   https://github.com/me-no-dev/arduino-esp32fs-plugin */
#define FORMAT_SPIFFS_IF_FAILED true

WiFiClient wifi;
extern MQTTClient mqttClient;
unsigned long lastMillis = 0;
// initialise tyhacMode
// TODO: rethink this, again, queues?
int tyhacMode = 0;
int tyhacButtonTestRequest = 0;
int tyhacButtonSubmitRequest = 0;

/*
*   connectWifi()
*   handles the wifi connectivity and returns bool based on result.
*/
bool connectWifi()
{
  changeRgbColor("orange");
  WiFi.setHostname(THINGNAME);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("TYHAC: Connecting to Wifi...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    changeRgbColor("orange");
    delay(500);
    changeRgbColor("orangeAlt");
    // note: I observed an issue where wifi would randomly not connect
    // rebooting seemed to fix the issue, this will track that
    // and reboot if needed.
    i++;
    if (i == 20)
    {
      Serial.println("TYHAC: Unable to connect to wifi, program will reboot");
      screenElemLoading("wifi failed");
      changeRgbColor("red");
      delay(5000);
      exit(0);
    }
  }

  return true;
}

/*
*   setup()
*   setup the device.
*/
void setup()
{
  M5.begin(
      true, // LCDEnable
      true, // SDEnable
      true, // SerialEnable, toggle for debugging
      false // I2CEnable
  );

  // Lcd display
  M5.Lcd.setBrightness(100);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(1);

  delay(100); // noticed this in other examples.

  // setup the screen elements, network connectivity and version
  screenElemHeaderFooter(2, 2); // init as red

  // Initialise RGB light strips
  setupRgb();

  // Setup the internal flash storage, this is used for certs
  if (!SPIFFS.begin(true))
  {
    Serial.println("TYHAC: Spiffs error setting up");
    return;
  }

  // Connect to Wifi
  if (connectWifi())
  {
    Serial.println("TYHAC: Wifi connected");
    screenElemLoading("wifi connected");
    screenElemHeaderFooter(0, 2);
    changeRgbColor("green");
    delay(2000);
    changeRgbColor("else");
  }

  // Configure NTP client, I generate some timestamps later
  // also note, some comments online noted SSL requirements for sync'd clock
  // I didn't notice this requirement for SSL when testing.
  setClock();

  // Change color to orange
  changeRgbColor("orange");

  // Connect to and subscribe to AWS IoT MQTT
  if (connectAWS())
  {
    Serial.println("TYHAC: AWS IoT MQTT connected");
    screenElemLoading("connecting to AWS MQTT");
    screenElemHeaderFooter(0, 0);
    changeRgbColor("green");

    subscribeAWS();
  }
  else
  {
    changeRgbColor("red");
    screenElemLoading("MQTT failed");
  }

  changeRgbColor("else");

  // Send a boot time message to the status topic
  messageBootup();
  messageRequestStats(); // Creates the dashboard screen

  Serial.println("TYHAC: AWS IoT MQTT bootup");
  screenElemLoading("bootup complete");
}

/*
*   loop()
*   the never ending loop.
*/
void loop()
{

  // get screen co-ords in touch
  TouchPoint_t pos = M5.Touch.getPressPoint();
  // register co-ords with our listeners
  buttonListeners(pos);

  // mqtt pub/sub
  mqttClient.loop();

  // 0 = passive
  if (tyhacMode == 0)
  {
    if (audioSensor())
    {
      changeRgbColor("blue");
      screenElemLoading("recording audio");
      // Capture audio, return true when done
      if (recordAudio())
      {
        screenElemLoading("audio complete");
        changeRgbColor("else");

        // Would sometimes drop after recording?
        screenElemLoading("reconnecting AWS MQTT");
        reconnectAWS();

        // request an S3 pre-signed URL
        messageRequestS3Url("passive");
        screenElemLoading("requesting AWS S3 url");
      }
    }
  }

  // 1 = clinician / field unit
  if (tyhacMode == 1)
  {
    if (tyhacButtonTestRequest == 1)
    {
      tyhacButtonTestRequest = 0; // reset the flag
      changeRgbColor("blue");
      screenElemLoading("recording audio");
      if (recordAudio())
      {
        screenElemLoading("audio complete");
        changeRgbColor("else");
        screenElemLoading("reconnecting AWS MQTT");
        reconnectAWS();
        messageRequestS3Url("test");
        screenElemLoading("requesting AWS S3 url");
      }
    }

    if (tyhacButtonSubmitRequest == 1)
    {
      tyhacButtonSubmitRequest = 0; // reset the flag
      changeRgbColor("blue");
      screenElemLoading("recording audio");
      if (recordAudio())
      {
        changeRgbColor("else");
        screenElemLoading("reconnecting AWS MQTT");
        reconnectAWS();
        messageRequestS3Url("submit");
        screenElemLoading("requesting AWS S3 url");
      }
    }
  }

  // heartbeat every minute pubs to mqtt
  if (millis() - lastMillis > 60000)
  {
    lastMillis = millis();
    messageHeartBeat();
    Serial.println("TYHAC: AWS IoT MQTT heartbeat, MODE = " + tyhacModeString(tyhacMode));
  }

  // The MQTT would drop occasionally and always after http interaction through widi
  // this just makes sure it reconnects.
  reconnectAWS();
}
