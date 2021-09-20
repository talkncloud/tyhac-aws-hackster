/*
*   Description:  AWS IoT MQTT pub/sub handles connectivity, call backs and message processing.
*  
*   Author:       Mick Jacobsson - (https://www.talkncloud.com)
*   Repo:         https://github.com/talkncloud/tyhac-aws-hackster
*/
#include "tyhac_mqtt.h"
#include "tyhac_uploads3.h"
#include "tyhac_ntp.h"
#include "tyhac_rgb.h"
#include "tyhac_ui.h"
#include "env.h"

WiFiClientSecure wifiClient = WiFiClientSecure();
MQTTClient mqttClient = MQTTClient(1536);
extern int tyhacMode;

/*
*   messageUploadS3(String statusS3, String filename, String mode, String modeType)
*   publishes the AWS S3 upload information to AWS IoT
*/
void messageUploadS3(String statusS3, String filename, String mode, String modeType)
{
  StaticJsonDocument<200> docIn;
  docIn["filename"] = filename;
  docIn["thing"] = THINGNAME;
  docIn["location"]["latitude"] = TYHAC_LAT;
  docIn["location"]["longitude"] = TYHAC_LON;
  docIn["timestamp"] = getTime();
  docIn["mode"] = mode;
  docIn["modeType"] = modeType;
  docIn["action"] = "uploads3";

  docIn["status"] = statusS3; // success / fail
  char jsonBuffer[512];
  serializeJson(docIn, jsonBuffer);
  // TODO: mqtt always disconnected after upload, something to do with MQTT and Wifi?
  reconnectAWS();
  // Send message
  mqttClient.publish(TYHAC_AWSIOT_PUB_STATUS, jsonBuffer);

  // TODO: Would be better to use a queue or something?
}

/*
*   tyhacModeString(int mode)
*   simply converts the mode of the thing to a string, the result is pushed into AWS IoT
*   and stored in dynamodb.
*/
String tyhacModeString(int mode)
{
  String tyhacModeString;
  if (tyhacMode == 0)
  {
    tyhacModeString = "passive";
  }
  else if (tyhacMode == 1)
  {
    tyhacModeString = "active";
  }
  else
  {
    tyhacModeString = "unknown";
  }

  return tyhacModeString;
}

/*
*   messageHandler(topic, paylaod)
*   This is the callback from a message, the payload is then parsed using the json lib. If the
*   action contains a keyword it will then perform the next action.
*
*   Source/Credit: https://aws.amazon.com/blogs/compute/building-an-aws-iot-core-device-using-aws-serverless-and-an-esp32/
*                  https://github.com/Savjee/arduino-aws-iot-starter-template/blob/master/arduino-aws-iot-starter-template/arduino-aws-iot-starter-template.ino
*/
void messageHandler(String &topic, String &payload)
{
  Serial.println("TYHAC: AWS IoT MQTT MSG handler called");
  DynamicJsonDocument doc(1536);
  deserializeJson(doc, payload);

  String action = doc["action"];
  String filename = doc["filename"];
  String mode = doc["mode"];
  String modeType = doc["modeType"];

  if (action == "uploads3")
  {
    Serial.println("TYHAC: AWS IoT MQTT MSG handler - upload s3");
    screenElemLoading("AWS S3 uploading");
    // TODO: Would be better to use a queue from the main program
    if (uploadS3(payload))
    {
      String status = "success";
      screenElemLoading("AWS S3 upload success");
      messageUploadS3(status, filename, mode, modeType);
    }
    else
    {
      String status = "fail";
      screenElemLoading("AWS S3 upload fail");
      messageUploadS3(status, filename, mode, modeType);
    }
  }
  else if (action == "prediction")
  {
    Serial.println("TYHAC: AWS IoT MQTT MSG handler - prediction");
    // Update display with prediction results
    String status = doc["status"];
    String predictClass = doc["prediction"]["class"];
    String predictPercent = doc["prediction"]["percent"];
    screenElemLoading("clear"); // remove loading messages
    screenCovidResult(status, predictClass, predictPercent, filename);
  }
  else if (action == "dashboard")
  {
    Serial.println("TYHAC: AWS IoT MQTT MSG handler - dashboard");
    screenElemLoading("AWS MQTT dashboard");
    String samples = doc["samples"];
    String sampledays = doc["sampledays"];
    String positive = doc["positive"];
    String negative = doc["negative"];
    String uptime = doc["uptime"];
    screenDashboard(samples, sampledays, positive, negative, uptime);
    screenElemLoading("clear"); // remove loading messages
  }
}

/*
*   reconnectAWS()
*   everytime the HTTP client would upload to S3 the MQTT client would disconnect and 
*   causes messages to fail. This hanldes the reconnect both in the main loop
*   as after certain HTTP interactions. 
*
*   TODO: need to investigate why this happens, found some info in the internet with others
*         having similar issues. unsure if resolved.
*/
void reconnectAWS()
{
  if (!mqttClient.connected())
  {
    Serial.println("TYHAC: AWS IoT MQTT disconnected...reconnecting...");
    screenElemLoading("AWS MQTT reconnecting");
    screenElemHeaderFooter(0, 1);
    mqttClient.connect(THINGNAME);
    mqttClient.onMessage(messageHandler);
    subscribeAWS(); // resub to messages
    screenElemHeaderFooter(0, 0);
  }
}

/*
*   connectAWS()
*   Connects to the AWS IoT MQTT endpoint using the certificates.
*
*   Source/Credit: https://github.com/Savjee/arduino-aws-iot-starter-template/blob/master/arduino-aws-iot-starter-template/arduino-aws-iot-starter-template.ino
*/
bool connectAWS()
{

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  // Reading the certs in from SPIFFS
  extern const uint8_t aws_root_ca_pem_start[] asm("_binary_certs_aws_root_ca_pem_start");
  // extern const uint8_t aws_root_ca_pem_end[] asm("_binary_certs_aws_root_ca_pem_end");

  extern const uint8_t tyhac_core_certificate_pem_start[] asm("_binary_utilities_AWS_IoT_registration_helper_output_files_tyhacthing_certificate_pem_start");
  // extern const uint8_t tyhac_core_certificate_pem_end[] asm("_binary_utilities_AWS_IoT_registration_helper_output_files_tyhac_core_certificate_pem_end");

  extern const uint8_t tyhac_core_private_key_start[] asm("_binary_utilities_AWS_IoT_registration_helper_output_files_tyhacthing_private_key_start");
  // extern const uint8_t tyhac_core_private_key_end[] asm("_binary_utilities_AWS_IoT_registration_helper_output_files_tyhac_core_private_key_end");

  // Connect with certs
  wifiClient.setCACert((const char *)aws_root_ca_pem_start);
  wifiClient.setCertificate((const char *)tyhac_core_certificate_pem_start);
  wifiClient.setPrivateKey((const char *)tyhac_core_private_key_start);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  mqttClient.begin(AWS_IOT_ENDPOINT, 8883, wifiClient);

  // Create a message handler
  mqttClient.onMessage(messageHandler);

  Serial.println("TYHAC: Connecting to AWS IoT MQTT");

  while (!mqttClient.connect(THINGNAME))
  {
    delay(1000);
  }

  if (!mqttClient.connected())
  {
    Serial.println("TYHAC: AWS IoT MQTT unable to connect");
    return false;
  }

  return true;
}

/*
*   subscribeAWS()
*   Subscribes to MQTT topics/
*/
void subscribeAWS()
{
  screenElemLoading("AWS MQTT subscribe");
  // sign url
  mqttClient.subscribe(TYHAC_AWSIOT_SUB_PRESIGN);
  // predict
  mqttClient.subscribe(TYHAC_AWSIOT_SUB_PREDICT);
  // stats
  mqttClient.subscribe(TYHAC_AWSIOT_SUB_STATS);

  Serial.println("TYHAC: AWS IoT MQTT subscribed");
  screenElemLoading("clear");
}

/*
*   messageRequestS3Url(mode)
*   publish message to AWS. this initiates the s3 pre-signed url request.
*/
void messageRequestS3Url(String tyhacButtonMode)
{
  StaticJsonDocument<200> doc;
  doc["action"] = "requesturl";
  doc["mode"] = tyhacModeString(tyhacMode);
  doc["modeType"] = tyhacButtonMode;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  mqttClient.publish(TYHAC_AWSIOT_PUB_PRESIGN, jsonBuffer);
}

/*
*   messageRequestStats()
*   publish message to AWS. this initiates the dashboard stats from lambda -> dynamodb.
*/
void messageRequestStats()
{
  StaticJsonDocument<200> doc;
  doc["action"] = "requeststats";
  doc["thing"] = THINGNAME;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  mqttClient.publish(TYHAC_AWSIOT_PUB_STATS, jsonBuffer);
}

/*
*   messageHeartBeat()
*   publishes a heartbeat to the status/events tables every x time in the main loop.
*/
void messageHeartBeat()
{
  StaticJsonDocument<200> docIn;
  docIn["thing"] = THINGNAME;
  docIn["location"]["latitude"] = TYHAC_LAT;
  docIn["location"]["longitude"] = TYHAC_LON;
  docIn["timestamp"] = getTime();
  docIn["action"] = "heartbeat";
  docIn["mode"] = tyhacModeString(tyhacMode);

  docIn["status"] = "success"; // success / fail
  char jsonBuffer[512];
  serializeJson(docIn, jsonBuffer);

  mqttClient.publish(TYHAC_AWSIOT_PUB_STATUS, jsonBuffer);
}

/*
*   messageBootup()
*   publishes a thing bootup even to be stored in dynamodb. helps to determine
*   uptime, crashes etc.
*/
void messageBootup()
{
  StaticJsonDocument<200> docIn;
  docIn["thing"] = THINGNAME;
  docIn["location"]["latitude"] = TYHAC_LAT;
  docIn["location"]["longitude"] = TYHAC_LON;
  docIn["timestamp"] = getTime();
  docIn["action"] = "bootup";
  docIn["mode"] = tyhacModeString(tyhacMode);

  docIn["status"] = "success"; // success / fail
  char jsonBuffer[512];
  serializeJson(docIn, jsonBuffer);

  mqttClient.publish(TYHAC_AWSIOT_PUB_STATUS, jsonBuffer);
}