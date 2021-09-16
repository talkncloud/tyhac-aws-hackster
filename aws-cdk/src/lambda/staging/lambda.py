"""
    Description: Performs staging of samples through various checks and buckets.

                 The staging confirms if the sample is a wav file, a cough
                 from there it augments and copies to staging and 
                 final buckets.

                 Trigger: OBJECT_CREATE in raw bucket

                 Note: this is a custom container...refer to repo.

    Author: Mick Jacobsson (https://www.talkncloud.com)
    Repo: https://github.com/talkncloud/tyhac-aws-hackster
    
"""
import json
import boto3
from botocore.config import Config
from botocore.exceptions import ClientError
from urllib.parse import urlparse
import os
import logging
import math
from pydub import AudioSegment
import decimal
import time

from coughvid.src.feature_class import features
from coughvid.src.DSP import classify_cough
from scipy.io import wavfile
import pickle

from coughvid.src.DSP import preprocess_cough

logger = logging.getLogger()
logging.basicConfig()
logger.setLevel(logging.INFO)

s3 = boto3.client('s3')
region = os.environ['REGION']
bucketRaw = os.environ['BUCKET_RAW']
bucketStage = os.environ['BUCKET_STAGE']
bucketFinal = os.environ['BUCKET_FINAL']
thingTopicStatus = os.environ.get('TOPIC_PUB_STATUS')
thingTopicPredict = os.environ.get('TOPIC_SUB_PREDICT')
confidence = os.environ.get('CONFIDENCE')
tableName = os.environ.get('DYNAMO_TABLE')

dynamodb = boto3.resource('dynamodb', region_name=region)
from boto3.dynamodb.conditions import And, Attr

def handler(event, context):
    """
    Entry point for lambda.
    """

    try:
        # the s3 bucket triggered key
        bucketKey = event['Records'][0]['s3']['object']['key']

        # Shift files around and return the local /tmp file
        localFile = downloadFile(bucketKey)
        
        if (localFile != None):
            fs, x = checkWav(localFile, bucketKey)

        if (fs != None):
            logger.info("TYHAC: found wave")

            if (predictCough(x, fs, localFile, bucketKey)):
                logger.info("TYHAC: cough found")

                # Retain the unaugmented file
                if (stageFile(bucketKey, localFile)):
                        logger.info("TYHAC: file staged")
        
                        augmentedFile = augmentWave(localFile)
                        if(augmentedFile != None):
                            logger.info("TYHAC: audio augmented")

                            # Store final file
                            if (finalFile(bucketKey, augmentedFile)):
                                logger.info("TYHAC: final file completed")

        return
    except Exception as e:
        logger.error("TYHAC: unable to stage - " + str(e))
        updateMqtt(bucketKey, "fail")
        return

def checkWav(localFile, bucketKey):
    """
    Reads in the locally downloaded file and confirms the lib can parse the file. if
    not this isn't a valid sample. possibly corrupt from the thing or other
    concern.
    """
    try:
        fs, x = wavfile.read(localFile)
        return fs, x
    except Exception as e:
        logger.error("TYHAC: not a wave - " + str(e))
        removeSample(localFile, bucketKey)
        return None, None

def predictCough(x, fs, localFile, bucketKey):
    """
    This loads the coughvid cough detection model and attempts to detect if this is a cough as a first
    pass. I like this because this isn't the TYHAC model. This doesn't check for covid-19, just
    a cough.

    Source/Credit: https://c4science.ch/diffusion/10770/repository/master/

    """

    try:
        loaded_model = pickle.load(open(os.path.join('coughvid', 'models', 'cough_classifier'), 'rb'))
        loaded_scaler = pickle.load(open(os.path.join('coughvid', 'models', 'cough_classification_scaler'), 'rb'))

        probability = classify_cough(x, fs, loaded_model, loaded_scaler)
        roundProb = math.floor(probability*100)

        if (roundProb > int(confidence)):
            logger.info("TYHAC: we gotta cough here: " + str(roundProb) + "%")
            return True
        else:
            logger.info("TYHAC: no cough, remove: " + str(roundProb) + "%")
            removeSample(localFile, bucketKey)
            return False

    except Exception as e:
        logger.error("TYHAC: error predict - " + str(e))
        removeSample(localFile, bucketKey)
        return False

def downloadFile(bucketKey):
    """
    Download the file from the trigger to tmp and return
    """
    try:
        # Download locally
        localFile = '/tmp/' + bucketKey
        s3.download_file(bucketRaw, bucketKey, localFile)
        return localFile
    except Exception as e:
        logger.error("TYHAC: error downloading file - " + str(e))
        return None

def stageFile(bucketKey, localFile):
    """
    Now that we have a sample to work with, we need to move copy it into the
    staging bucket, remove it from the raw bucket and then copy it again
    to the final bucket. The purpose here is to retain an non augmented
    sample in case we stuff it and someone wants to go back and
    process the staged dataset.
    """

    try:
        # Using resource intead of client, didn't have luck with put_object and client
        s3_resource = boto3.resource('s3')
        
        # Upload
        s3_resource.meta.client.upload_file(localFile, bucketStage, bucketKey)
        
        # Delete from raw
        s3.delete_object(Bucket=bucketRaw, Key=bucketKey)

        # logger.info("TYHAC: file has been staged")
        return True

    except Exception as e:
        logger.error("TYHAC: error staging file - " + str(e))
        return False
    
def finalFile(bucketKey, augmentedFile):
    """
    Simply puts the augmented file into the final bucket. 
    """
    try:
        # Using resource intead of client, didn't have luck with put_object and client
        s3_resource = boto3.resource('s3')
        # Upload
        s3_resource.meta.client.upload_file(augmentedFile, bucketFinal, bucketKey)
        return True
    except Exception as e:
        logger.error("TYHAC: error moving final file - " + str(e))
        return False

class DecimalEncoder(json.JSONEncoder):
    """
    Convert json decimal from dynamo to int
    Source: https://stackoverflow.com/questions/40033189/using-boto3-in-python-to-acquire-results-from-dynamodb-and-parse-into-a-usable-v
    """
    def default(self, o):
        if isinstance(o, decimal.Decimal):
            return int(o)
        if isinstance(o, set):  #<---resolving sets as lists
            return list(o)
        return super(DecimalEncoder, self).default(o)

def getRecord(recordId):
    """
    Need to pull the record from dynamoDB. The filename is unique, we'll use that
    to pull the record information which will be used to update MQTT and then
    back to dynamodb again.
    """

    try:
        uid = recordId.split('.')[0]

        table = dynamodb.Table(tableName)

        response = table.scan(
            FilterExpression=Attr("filename").eq(uid)
        )

        return json.dumps(response['Items'][0], cls=DecimalEncoder)

    except Exception as e:
        logger.error("TYHAC: error getting dynamodb record - " + str(e))
        return

def updateMqtt(bucketKey, stageStatus):
    """
    This will take the exisiting status/event information from dynamodb and add the status of
    the staging to the exisitng record. There won't be a line item for staging, just a
    field. The client is also sent back info on the predict sub if fail.
    """
    try:

        uid = bucketKey.split('.')[0]
        record = json.loads(getRecord(uid))
                
        # Build payload
        record['staging'] = stageStatus

        # # setup the mqtt client
        mqtt = boto3.client('iot-data')

        # Update the status table
        response = mqtt.publish (
            topic = thingTopicStatus,
            qos = 0,
            payload = json.dumps(record)
        )

        # Only need to update the client if its a fail, this would mean, invalid file or no cough
        # as this bypasses the tyhac predictor. 
        if (stageStatus == "fail"):
            timestamp = int(time.time())
            thingName = record['thing']
            location = record['location']
            # json payload
            jsonPayload = json.dumps({
                "timestamp": timestamp,
                "thing": thingName,
                "action": "prediction",
                "prediction": {},
                "location": location,
                "status": stageStatus,
                "filename": uid
            })
            # Update the status table
            response = mqtt.publish (
                topic = thingTopicPredict,
                qos = 0,
                payload = jsonPayload
            )

    except Exception as e:
        logger.error("TYHAC: error updating MQTT - " + str(e))
        return

def augmentWave(localFile):
    """
    The audio recorded is pretty low volume, we'll augment it a bit. Try out
    a few otheres here like low/high pass filters. Work in progress.
    """
    try:
        # load file
        wav_file = AudioSegment.from_wav(localFile)

        # low pass filter
        # low_pass = wav_file.low_pass_filter(1000)

        # high pass filter
        # high_pass = low_pass.high_pass_filter(1000)

        # ESP32 SPM1423 records pretty low volume, lets increase
        augment_wav_file = wav_file + 10 

        # Export louder audio file 
        augment_wav_file.export(out_f = "/tmp/augmented_file.wav", format = "wav")

        # logger.info("TYHAC: audio augmented")
        return "/tmp/augmented_file.wav"

    except Exception as e:
        logger.error("TYHAC: error augmenting - " + str(e))
        return None

def removeSample(localFile, bucketKey):
    """
    This handles files when the sample hasn't been detected as a wav file or a cough. This will mean
    the IoT device will show this as invalid.
    """
    try:
        # Remove the file as it's not a cough or a wav
        s3.delete_object(Bucket=bucketRaw, Key=bucketKey)
        # Remove local file (gets cleaned anyways)
        os.remove(localFile)
        # Return the result to the MQTT
        updateMqtt(bucketKey, "fail")
    except Exception as e:
        logger.error("TYHAC: error removing samples - " + str(e))


if __name__ == "__main__":
    # handler(None, None)
    # localFile = "raw_sd.wav"
    # fs, x = checkWav(localFile, "raw_sd.wav")
    # predictCough(x, fs, localFile, localFile)
    # augmentWave("raw_sd.wav")

    # getRecord("tyhac-iotdata", "847f6aeb-8b24-42b3-9bf8-a8bac17b4fd2")
    handler(None, None)
