## Welcome to the Tyhac IoT arduino firmware

This will desciribe the requirements to configure your M5 Core2 AWS EduKit device with the tyhac firmware.

## Prerequisties

You will need to have deployed the [AWS CDK project]('../aws-cdk/') or manually created the resources required for this work such as:

* AWS IoT Core
* AWS S3

## Device provisioning

To use the project you will need to provision the device in AWS IoT core to allow secure communication. This will register the device as a thing, create certificates and deploy a default policy. The certifactes required for secure communication will need to go into a folder (utilities/AWS_IoT_registration_helper/output_files) in this project and will then loaded into the SPIFFS to be used for secure comms. 

To assist in this process there is a helper script provided. This script uses the standard AWS cli commands (```aws iot```) to interact with AWS to handle the provisioning. This method simulates what occurs when you do this manually through the console.

1. Start a new shell terminal
2. Authenticate using the AWS cli
3. ./utilities/registration_helper.sh

TODO: elliptic curve algo, this would not work with the WifiSecure library, I have found a custom AWS lib that may solve this issue.

### Confirm using AWS IoT Core

1. Login to the AWS console via a web browser
2. Navigate to AWS IoT core
3. Confirm a new thing has been registered with "tyhacthing" as the name

## Platform IO

This project requires platform.io to be used for development. The project has libraries external that will need to be installed, platform.io handles this and in general is nicer dev experience.

1. Load platform.io in vscode
2. Go to open project

Note: You may need to change the monitor port in the platformio.ini for your setup.

## Environment file

Inside the lib/env folder is an env.sample.h, update the configuration to match your environment:

1. rename the file to env.h
2. Compile and upload the project.
3. Open the monitor port

Note: It's likely you don't know what your AWS IoT core endpoint is, run the following to retrieve your endpoint address:
```
aws iot describe-endpoint --endpoint-type iot:Data-ATS
```

## Monitor output

The serial output of the device is enabled by default. A number of messages have been configured to show you the current status of the various functions on the device. You'll notice that every minute a heartbeat message is sent. 
