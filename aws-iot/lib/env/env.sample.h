// OPTIONAL
#define THINGNAME "tyhacthing" // the thing will be registered with this name and used throughout with this name
#define TYHAC_AUDIO_SAMPLE "/tyhac_sample.wav" // file saved on sd card

const char TYHAC_LAT[] = "-19.258965"; // your latitude, future use gps unit
const char TYHAC_LON[] = "146.816956"; // your longitude, future use gps unit

// MUST BE UPDATED

// using the aws cli: 'aws sts get-caller-identity' will give you your account id
const char TYHAC_AWS_BUCKET[] = "tyhac-ACCOUNTID-raw.s3.amazonaws.com"; // if it's a new bucket you may need to add the region e.g. ap-southeast-2

const char WIFI_SSID[] = "SSID";
const char WIFI_PASSWORD[] = "SECRET";

// using the aws cli: 'aws iot describe-endpoint --endpoint-type iot:Data-ATS' will give you your endpoint
const char AWS_IOT_ENDPOINT[] = "talkncloud.iot.ap-southeast-2.amazonaws.com"; // remember to update region
