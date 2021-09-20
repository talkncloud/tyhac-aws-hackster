#include <Arduino.h>
#include <M5Core2.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include <freertos/ringbuf.h>
#include <WiFiClientSecure.h>
#include <tyhac_ui.h>

#define TYHAC_AWSIOT_SUB_PRESIGN "tyhac/sub/presign"
#define TYHAC_AWSIOT_SUB_PREDICT "tyhac/sub/predict"

#define TYHAC_AWSIOT_PUB_PRESIGN "tyhac/pub/presign"
#define TYHAC_AWSIOT_PUB_PREDICT "tyhac/pub/predict"
#define TYHAC_AWSIOT_PUB_STATUS "tyhac/pub/status" // capture all status events

#define TYHAC_AWSIOT_PUB_STATS "tyhac/pub/stats" // dashboard
#define TYHAC_AWSIOT_SUB_STATS "tyhac/sub/stats" // dashboard

bool connectAWS();
void reconnectAWS();
void subscribeAWS();

void messageBootup();
void messageHeartBeat();
void messageRequestS3Url(String tyhacButtonMode);
void messageRequestStats();

String tyhacModeString(int mode);