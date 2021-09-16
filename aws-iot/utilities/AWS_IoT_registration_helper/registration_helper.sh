#!/bin/sh

# Source: https://iot-device-management.workshop.aws/en/provisioning-options/provisioning-with-api.html

# creates the device certificates
THING_NAME=tyhac-core

# create a thing in the thing registry
aws iot create-thing --thing-name $THING_NAME

aws iot create-keys-and-certificate --set-as-active \
 --public-key-outfile output_files/$THING_NAME.public.key \
 --private-key-outfile output_files/$THING_NAME.private.key \
 --certificate-pem-outfile output_files/$THING_NAME.certificate.pem > output_files/provisioning-claim-result.json

# output values from the previous call needed in further steps
CERTIFICATE_ARN=$(jq -r ".certificateArn" output_files/provisioning-claim-result.json)
CERTIFICATE_ID=$(jq -r ".certificateId" output_files/provisioning-claim-result.json)
echo $CERTIFICATE_ARN
echo $CERTIFICATE_ID

# create an IoT policy
POLICY_NAME=${THING_NAME}_Policy
aws iot create-policy --policy-name $POLICY_NAME \
  --policy-document '{"Version":"2012-10-17","Statement":[{"Effect":"Allow","Action": "iot:*","Resource":"*"}]}'

# attach the policy to your certificate
aws iot attach-policy --policy-name $POLICY_NAME \
  --target $CERTIFICATE_ARN

# attach the certificate to your thing
aws iot attach-thing-principal --thing-name $THING_NAME \
  --principal $CERTIFICATE_ARN