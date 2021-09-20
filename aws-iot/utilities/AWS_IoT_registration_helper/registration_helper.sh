#!/bin/sh

# Creates the certs in AWS and outputs directory. Certs are loaded onto the device.
# Note: Deploy the CDK stack before running this stack.

# Source: https://iot-device-management.workshop.aws/en/provisioning-options/provisioning-with-api.html

# creates the device certificates
THING_NAME=tyhacthing

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

# Find the policy from our deployed CDK stack and associate to our device
POLICY_NAME=$(aws iot list-policies | jq -r .policies[].policyName | grep tyhac)

# attach the policy to your certificate
aws iot attach-policy --policy-name "$POLICY_NAME" \
  --target $CERTIFICATE_ARN

# attach the certificate to your thing
aws iot attach-thing-principal --thing-name $THING_NAME \
  --principal $CERTIFICATE_ARN