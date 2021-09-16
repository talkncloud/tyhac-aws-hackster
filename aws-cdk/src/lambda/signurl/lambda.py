"""
    Description: Generates a pre-signed url and then updates the MQTT topic
                 with the result to be consumed by the thing.

    Author: Mick Jacobsson (https://www.talkncloud.com)
    Repo: https://github.com/talkncloud/tyhac-aws-hackster

"""

import logging
import boto3
import os
import json
from botocore.config import Config
from botocore.exceptions import ClientError
from urllib.parse import urlparse
import uuid

def create_presigned_url(bucket_name, object_name, expiration=120):
    """Generate a presigned URL to share an S3 object

    :param bucket_name: string
    :param object_name: string
    :param expiration: Time in seconds, currently set to 2 minutes
    :return: Presigned URL as string. If error, returns None.
    """

    # Generate a presigned URL for the S3 object
    s3_client = boto3.client('s3', config=Config(signature_version='s3v4'))
    try:
        response = s3_client.generate_presigned_url('put_object',
                                                    Params={'Bucket': bucket_name,
                                                            'Key': object_name},
                                                    ExpiresIn = expiration,
                                                    HttpMethod = 'PUT'
                                                    )
    except ClientError as e:
        logging.error("error signing url: %s", e)
        return None

    # The response contains the presigned URL
    return response

def handler(event, context):
    """
    Entry point for lambda.
    """
    try:
        incPayload = json.dumps(event)
        incPayload = json.loads(incPayload)
        mode = incPayload['mode']
        modeType = incPayload['modeType']
        # setup the mqtt client
        mqtt = boto3.client('iot-data')
        # grab the bucket name
        # This is bucket from the env variable
        bucketName = os.environ.get('BUCKET_RAW')
        thingTopic = os.environ.get('TOPIC_PUB')
        # grab the object, lambda will now generate the uid's for us
        objectKey = str(uuid.uuid4())
        objectKeyWav = objectKey + '.wav'
        # call the pre-signed
        signurl = create_presigned_url(bucketName, objectKeyWav)
        parseUrl = urlparse(signurl)

        # host = parseUrl.hostname
        path = parseUrl.query
        filename = parseUrl.path

        # print to log
        logging.info("TYHAC: success: %s - %s", bucketName, objectKey)

        # json payload
        jsonPayload = json.dumps({
            "action": "uploads3",
            "mode": mode,
            "modeType": modeType,
            "filename": objectKey,
            "path": filename + "?" + path
        })

        # publish to mqtt
        response = mqtt.publish (
            topic = thingTopic,
            qos = 0,
            payload = jsonPayload
        )

    except Exception as e:
        logging.error("TYHAC: error calling sign url: %s", e)
        return None

if __name__ == "__main__":
    """ 
    Used for local testing
    """
    account = boto3.client('sts').get_caller_identity().get('Account')
    mqtt = boto3.client('iot-data')
    bucketName = "tyhac-" + account + "-raw"
    objectKey = str(uuid.uuid4()) + '.wav'
    signurl = create_presigned_url(bucketName, objectKey)
    print(signurl)
    # format the response
    parseUrl = urlparse(signurl)
    path = parseUrl.query
    filename = parseUrl.path

    print(filename + "?" + path)
    
    # response = mqtt.publish (
    #     topic = 'tyhac/sub/url',
    #     qos = 0,
    #     payload = filename + "?" + path
    # )
