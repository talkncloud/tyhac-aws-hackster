"""
    Description: Queries DynamoDB for stats on events / status data from TYHAC. The end result publishes a
              the payload to the MQTT topic the things are subscribing to.

    Author: Mick Jacobsson (https://www.talkncloud.com)
    Repo: https://github.com/talkncloud/tyhac-aws-hackster

"""

import logging
import boto3
import os
import json
from botocore.config import Config
from botocore.exceptions import ClientError
import decimal
import time

logger = logging.getLogger()
logging.basicConfig()
logger.setLevel(logging.INFO)

tableName = "tyhac-iotdata"
region = "ap-southeast-2"
dynamodb = boto3.resource('dynamodb', region_name=region)
thingTopicStats = os.environ.get('TOPIC_SUB')

from boto3.dynamodb.conditions import And, Attr

def handler(event, context):
    """
    Entry point for lambda.
    """
    try:
        # the thing will send the name from the device, helps us add
        # more devices later.
        incPayload = json.dumps(event)
        incPayload = json.loads(incPayload)
        thingName = incPayload['thing']

        stats = getRecord(thingName, "prediction")
        bootup = getRecord(thingName, "bootup")
        
        stats = processRecords(stats, "prediction")
        bootup = processRecords(bootup, "bootup")
        
        payload = mergePayloads(stats, bootup)
        
        updateMqtt(payload)
        
        logging.info("TYHAC: stats handler completed")

        return

    except Exception as e:
        logging.error("TYHAC: error calling handler" + str(e))
        return None

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
            
        
def processRecords(payload, action):
    """
    Builds out the stats for the thing dashboard / home display. This needs a bit of rethink
    as these types of queries won't go too well on dynamodb. Maybe moving towards a
    stream for counters would be better as it scales.
    """
    
    try:
    
        records = json.loads(payload)
        
        samples = len(records)
        
        negative = 0
        
        positive = 0
    
        timestamps = []
    
        # Get positive negative cases    
        for testResult in records:
            timestamps.append(testResult['timestamp'])
            if ('prediction' in testResult):
                if ('class' in testResult['prediction']):
                    if (testResult['prediction']['class'] == "negative"):
                        negative += 1
                    if (testResult['prediction']['class'] == "positive"):
                        positive += 1
    
        # Max timestamp
        now = int(time.time()) # epoch now
        lastSample = max(timestamps) # biggest num
        lastSample = now - lastSample
        lastSample = lastSample // 86400 # days
        
        if (action == "bootup"):
            return lastSample
        
        payload = {}
        payload['samples'] = samples
        payload['sampledays'] = lastSample
        payload['positive'] = positive
        payload['negative'] = negative
        
        return payload
    
    except Exception as e:
        logger.error("TYHAC: error processing records - " + str(e))
        return
    

def mergePayloads(stats, bootup):
    """
    Just want to merge the two payloads into one before mqtt.
    """
    try:
        stats = stats
        bootup = bootup
    
        jsonPayload = json.dumps({
            "samples": stats['samples'],
            "sampledays": stats['sampledays'],
            "positive": stats['positive'],
            "negative": stats['negative'],
            "action": "dashboard", # arduino callback will look for this
            "uptime": bootup
        })
        
        return jsonPayload
    
    except Exception as e:
        logger.error("TYHAC: error merging payloads - " + str(e))
        return

def getRecord(recordId, action):
    """
    Need to pull the record from dynamoDB. The filename is unique, we'll use that
    to pull the record information which will be used to update MQTT and then
    back to dynamodb again.
    """

    try:
        table = dynamodb.Table(tableName)

        response = table.scan(
            FilterExpression=Attr("thing").eq(recordId) and Attr("action").eq(action)
        )

        return json.dumps(response['Items'], cls=DecimalEncoder)

    except Exception as e:
        logger.error("TYHAC: error getting dynamodb record - " + str(e))
        return

def updateMqtt(payload):
    """
    Publishes an MQTT payload to the desired topic.
    """
    try:
        # setup the mqtt client
        mqtt = boto3.client('iot-data')

        # Update the status table
        response = mqtt.publish (
            topic = thingTopicStats,
            qos = 0,
            payload = payload
        )

    except Exception as e:
        logger.error("TYHAC: error updating MQTT - " + str(e))
        return
       
if __name__ == "__main__":
    """ 
    Use for local testing
    Note: you'll need to update the env manually or set at the start.
    """
    
    handler(None, None)
    