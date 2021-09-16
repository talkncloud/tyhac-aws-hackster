"""
    Description: Loads the TYHC SageMaker trained model exported with fast-ai
                 and returns predictions.

                 I'ved tried to keep this as similiar as the SageMaker 
                 endpoint script. If preferred deploy an SM 
                 endpoint and lambda to that instead.

                 The source is mostly from AWS examples online, refer
                 to the TYHAC SM folder for more details.

                 Trigger: OBJECT CREATED final bucket

                 Model: Update the model in the folder as you train with
                        alt parameters and get better results.

                 Note: this is a custom container...refer to repo.

    Author: Mick Jacobsson (https://www.talkncloud.com)
    Repo: https://github.com/talkncloud/tyhac-aws-hackster
    
"""

import logging, os, time
import json
import decimal
from fastai.vision.all import *
import boto3

JSON_CONTENT_TYPE = 'application/json'

logger = logging.getLogger()
logging.basicConfig()
logger.setLevel(logging.INFO)

s3 = boto3.client('s3')
region = os.environ['REGION']
tableName = os.environ.get('DYNAMO_TABLE')
thingTopicStatus = os.environ.get('TOPIC_PUB_STATUS')
thingTopicPredict = os.environ.get('TOPIC_PUB_PREDICT')
thingTopicPredict = "tyhac/sub/predict"
dynamodb = boto3.resource('dynamodb', region_name=region)
from boto3.dynamodb.conditions import And, Attr
bucketFinal = os.environ['BUCKET_FINAL']

def handler(event, context):
    """
    Entry point for lambda.
    """
    try:
        bucketKey = event['Records'][0]['s3']['object']['key']
        recordId = bucketKey.split('.')[0]
        logger.info("TYHAC: begin for sample - " + recordId)
        model = model_fn("model")

        # load the model
        if (model):
            audio = input_fn(bucketFinal + '/' + bucketKey, "text/csv")
            if (audio):  
                predict = predict_fn(audio, model)
                if (predict):
                    output = output_fn(predict)

        # format the results
        if (output != None):
            record = json.loads(getRecord(recordId))
            mqtt = updateMqtt(bucketKey, record, "success", output)
            logger.info("TYHAC: MQTT client updated")
        else:
            record = json.loads(getRecord(recordId))
            mqtt = updateMqtt(bucketKey, record, "fail", None)
            logger.info("TYHAC: MQTT client updated")

        return
    
    except Exception as e:
        logger.error("TYHAC: error - " + str(e))
        record = json.loads(getRecord(recordId))
        mqtt = updateMqtt(bucketKey, record, "fail", None)
        logger.info("TYHAC: MQTT client updated")
        return

def model_fn(model_dir):
    """
    Loads model into memory.
    """
    logger.info('model_fn')
    path = Path(model_dir)
    learn = load_learner(model_dir + '/export.pkl')
    logger.info('TYHAC: Model has been loaded')
    return learn

def input_fn(request_body, request_content_type):
    """
    Download the input from s3 and return
    Source/credit: https://aws.amazon.com/blogs/machine-learning/applying-voice-classification-in-an-amazon-connect-telemedicine-contact-flow/
    """
    logger.info('TYHAC: Deserializing the input data.')
    
    if request_content_type == 'text/csv':
        tmp=request_body
        bucket=tmp[:tmp.index('/')]
        obj=tmp[tmp.index('/')+1:]
        file = s3.download_file(bucket, obj, '/tmp/audioinput.wav')
        return '/tmp/audioinput.wav'
    
    raise Exception('Requested unsupported ContentType in content_type: {}'.format(request_content_type))

def predict_fn(input_object, model):
    """
    Model prediciton from out loaded model and input.
    """
    logger.info("TYHAC: Calling model for predict")
    start_time = time.time()
    predict_class,predict_idx,predict_values = model.predict(input_object)
    print("--- Inference time: %s seconds ---" % (time.time() - start_time))
    print(f'Predicted class is {str(predict_class)}')
    print(f'Predict confidence score is {predict_values[predict_idx.item()].item()}')
    return dict(class_name = str(predict_class),
        confidence = predict_values[predict_idx.item()].item())

def output_fn(prediction, accept=JSON_CONTENT_TYPE): 
    """
    Return the results.
    """       
    logger.info('TYHAC: Serializing the predicted output.')
    if accept == JSON_CONTENT_TYPE: return json.dumps(prediction), accept
    raise Exception('Requested unsupported ContentType in Accept: {}'.format(accept)) 

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

def updateMqtt(bucketKey, record, predictStatus, predictResult):
    """
    Publishes an MQTT payload to the desired topic.
    """
    try:

        uid = bucketKey.split('.')[0]
                
        # UTC epoch seconds
        timestamp = int(time.time())
        
        thingName = record['thing']
        location = record['location']

        prediction = {}
        if (predictResult != None):
            predictResult = json.loads(predictResult[0])
            prediction['class'] = predictResult['class_name']
            prediction['percent'] = predictResult['confidence'] - predictResult['confidence'] % 0.01 # no rounding, two places

        # # setup the mqtt client
        mqtt = boto3.client('iot-data')

        # json payload
        jsonPayload = json.dumps({
            "timestamp": timestamp,
            "thing": thingName,
            "action": "prediction",
            "prediction": prediction,
            "location": location,
            "status": predictStatus, # bring in as var
            "filename": uid
        })

        # Update the status table
        response = mqtt.publish (
            topic = thingTopicStatus,
            qos = 0,
            payload = jsonPayload
        )

        # Update the status table
        response = mqtt.publish (
            topic = thingTopicPredict, #sub/predict
            qos = 0,
            payload = jsonPayload
        )

    except Exception as e:
        logger.error("TYHAC: error updating MQTT - " + str(e))
        return

if __name__ == "__main__":
    """
    Local testing.
    """
    # model = model_fn('tyhac-fastai-2021-08-22-23-10-40-004')
    # print(int(time.time()))
    # record = json.loads(getRecord("847f6aeb-8b24-42b3-9bf8-a8bac17b4fd2"))
    # predict = handler(None, None)
    # updateMqtt(None, record, "success", predict)
    # # print(record['filename'])

    handler(None, None)