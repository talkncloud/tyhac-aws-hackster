/*
* Description:  Uses the WifiSecureClient to upload files to AWS S3 using a 
*               pre-signed URL.
*               
* Author:       Mick Jacobsson - (https://www.talkncloud.com)
* Repo:         https://github.com/talkncloud/tyhac-aws-hackster
*/
#include <tyhac_uploads3.h>
#include "env.h"

File fileread;

/*
*   uploadS3()
*   OK, uploading is really bloody hard. This will take the file from the SD and upload to the s3 bucket.
*
*   TIPS: The headers matter! You don't need to worry about content length including the headers.
*   The sampling rate and bits etc can be adjusted in the .h for this program.
*
*   If you have file issues e.g. corrupt header, inspect the file (s3) using linux/mac with:
*   od -bc uploaded.wav| head
*   
*
*   Source/Credit:  https://github.com/m5stack/M5Core2
*                   https://github.com/aws/amazon-freertos/blob/main/demos/coreHTTP/http_demo_s3_upload.c#L539-L632
*
*/
bool uploadS3(String payload)
{

  DynamicJsonDocument doc(1536);
  deserializeJson(doc, payload);

  // Grab the json payload items we need
  const char *uid = doc["uid"];
  const char *path = doc["path"];

  const char *filename = TYHAC_AUDIO_SAMPLE;

  fileread = SD.open(filename, FILE_READ);

  String filesize = String(fileread.size());

  WiFiClientSecure client;

  // Load in the cert
  extern const uint8_t aws_root_ca_pem_start[] asm("_binary_certs_aws_root_s3_ca_pem_start");

  // Use the cert to establish a connection
  client.setCACert((const char *)aws_root_ca_pem_start);

  // This is the bucketname with the region. If you've just created a bucket (last 24 hours) you need region.
  if (!client.connect(TYHAC_AWS_BUCKET, 443))
  {
    Serial.println("TYHAC: AWS S3 Upload connect failed");
    return false;
  }
  else
  {

    // DO NOT CHANGE THE HEADERS!!
    String allHeaders = "PUT " + String(path) + " HTTP/1.1\r\n";
    allHeaders += "Host: " + String(TYHAC_AWS_BUCKET) + "\r\n";
    // allHeaders += "Content-Type: text/plain; charset=UTF-8\r\n"; // text/plain for testing
    allHeaders += "Content-Type: audio/wave\r\n"; // text/plain for testing
    allHeaders += "Content-Length: " + filesize + "\r\n";

    String conLength = filesize;

    client.println(allHeaders);
    // Serial.println(allHeaders);
    // Serial.println("Content-Length: " + String(conLength));

    // DO NOT ADD a trailing println here, it will end up in the wav

    // Use a file buffer, splitting it into chunks makes it much faster
    const int bufSize = 1024;
    byte clientBuf[bufSize];
    int clientCount = 0;

    while (fileread.available())
    {
      clientBuf[clientCount] = fileread.read();
      clientCount++;
      if (clientCount > (bufSize - 1))
      {
        client.write((const byte *)clientBuf, bufSize);
        clientCount = 0;
      }
    }
    if (clientCount > 0)
    {
      client.write((const byte *)clientBuf, clientCount);
    }

    client.println();

    fileread.close();

    // Read the output from the request
    while (client.connected())
    {
      String line = client.readStringUntil('\n');
      String httpCode = line.substring(9, 12);
      if (httpCode == "200")
      { // TODO: is switch avail?
        // We can terminate, we don't need the response once its 200
        client.stop();
        Serial.println("TYHAC: AWS S3 Upload success");
        return true;
      }
      else
      {
        // Non 200, we want the full output for troubleshooting
        Serial.println(line); // uncomment me to see AWS response e.g. 400 with text reasons
        Serial.println("TYHAC: AWS S3 Upload file upload failed");
        return false;
      }
    }
    client.println();
  } // end connected

  // disconnect when done
  client.stop();

  return true;
}