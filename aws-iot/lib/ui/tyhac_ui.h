#include "M5Core2.h"
#include "MQTTClient.h"
#include "ArduinoJson.h"

// modes 0 = passive, 1 = active / clinician
extern int tyhacMode;

extern int tyhacButtonTestRequest;
extern int tyhacButtonSubmitRequest;

void buttonListeners(TouchPoint_t pos);

void screenElemHeaderFooter(int statusWifi, int statusAws);
void screenDashboard(String samples, String sampledays, String positive, String negative, String uptime);
void screenCovidResult(String status, String predictClass, String predictPercent, String filename);
void screenClinician();
void screenElemLoading(String loadingMessage);
