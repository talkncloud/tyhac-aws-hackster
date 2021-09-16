import * as dynamo from "@aws-cdk/aws-dynamodb";
import { AttributeType } from "@aws-cdk/aws-dynamodb";
import * as iam from "@aws-cdk/aws-iam";
import { ServicePrincipal } from "@aws-cdk/aws-iam";
import * as iot from "@aws-cdk/aws-iot";
import * as lambda from "@aws-cdk/aws-lambda";
import { DockerImageCode } from "@aws-cdk/aws-lambda";
import { S3EventSource } from "@aws-cdk/aws-lambda-event-sources";
import * as s3 from "@aws-cdk/aws-s3";
import * as cdk from "@aws-cdk/core";
import { Construct, Stack, StackProps } from "@aws-cdk/core";

export class TyhacStack extends Stack {
  constructor(scope: Construct, id: string, props: StackProps = {}) {
    super(scope, id, props);

    // Dynamodb - datastore table
    const dynamoIoTdb = new dynamo.Table(this, "dynamo-store", {
      encryption: dynamo.TableEncryption.AWS_MANAGED,
      tableName: "tyhac-iotdata",
      partitionKey: {
        name: "timestamp",
        type: AttributeType.NUMBER,
      },
      sortKey: {
        name: "thing",
        type: AttributeType.STRING,
      },
    });

    // s3 bucket - raw
    const bucketRaw = new s3.Bucket(this, "bucket-raw", {
      bucketName: "tyhac-" + props.env?.account + "-raw",
      encryption: s3.BucketEncryption.S3_MANAGED,
      blockPublicAccess: s3.BlockPublicAccess.BLOCK_ALL,
    });

    // s3 bucket - staging
    const bucketStaging = new s3.Bucket(this, "bucket-staging", {
      bucketName: "tyhac-" + props.env?.account + "-staging",
      encryption: s3.BucketEncryption.S3_MANAGED,
      blockPublicAccess: s3.BlockPublicAccess.BLOCK_ALL,
    });

    // s3 bucket - final
    const bucketFinal = new s3.Bucket(this, "bucket-final", {
      bucketName: "tyhac-" + props.env?.account + "-final",
      encryption: s3.BucketEncryption.S3_MANAGED,
      blockPublicAccess: s3.BlockPublicAccess.BLOCK_ALL,
    });

    // Lambda - PreSign S3 URL
    const lambdaSignUrl = new lambda.Function(this, "lambda-sign", {
      functionName: "tyhac-signurl",
      description:
        "tyhac generates pre-signed url for staging audio files from the AWS IoT Core",
      runtime: lambda.Runtime.PYTHON_3_8,
      code: lambda.Code.fromAsset("./src/lambda/signurl"),
      handler: "lambda.handler",
      environment: {
        BUCKET_RAW: bucketRaw.bucketName,
        TOPIC_PUB: "tyhac/sub/presign",
      },
    });

    // Lambda - Bucket staging
    const lambdaStaging = new lambda.DockerImageFunction(
      this,
      "lambda-staging",
      {
        functionName: "tyhac-staging",
        description: "tyhac s3 bucket staging and processing",
        code: DockerImageCode.fromImageAsset("./src/lambda/staging/"),
        timeout: cdk.Duration.seconds(120),
        environment: {
          REGION: Stack.of(this).region,
          BUCKET_RAW: bucketRaw.bucketName,
          BUCKET_STAGE: bucketStaging.bucketName,
          BUCKET_FINAL: bucketFinal.bucketName,
          DYNAMO_TABLE: dynamoIoTdb.tableName,
          TOPIC_PUB_STATUS: "tyhac/pub/status",
          TOPIC_SUB_PREDICT: "tyhac/sub/predict",
          CONFIDENCE: "3",
        },
      }
    );

    // Lambda - Bucket predictor
    const lambdaPredictor = new lambda.DockerImageFunction(
      this,
      "lambda-predictor",
      {
        functionName: "tyhac-predictor",
        description: "tyhac covid-19 predictor",
        code: DockerImageCode.fromImageAsset("./src/lambda/predictor/"),
        timeout: cdk.Duration.seconds(300),
        memorySize: 512,
        environment: {
          REGION: Stack.of(this).region,
          BUCKET_FINAL: bucketFinal.bucketName,
          DYNAMO_TABLE: dynamoIoTdb.tableName,
          TOPIC_PUB_STATUS: "tyhac/pub/status",
          TOPIC_SUB_PREDICT: "tyhac/sub/predict",
        },
      }
    );

    // Lambda - Dashboard stats
    const lambdaStats = new lambda.Function(this, "lambda-stats", {
      functionName: "tyhac-stats",
      description: "tyhac queries dynamo for stats and sends to mqtt",
      runtime: lambda.Runtime.PYTHON_3_8,
      code: lambda.Code.fromAsset("./src/lambda/stats"),
      handler: "lambda.handler",
      environment: {
        TOPIC_SUB: "tyhac/sub/stats",
      },
    });

    // IoT Core - Policy
    const iotThingPolicy = new iot.CfnPolicy(this, "iot-policy", {
      // policyName: "tyhac-policy", // naming did not support updates, argh, avoid
      policyDocument: {
        Version: "2012-10-17",
        Statement: [
          {
            Effect: "Allow",
            Action: "iot:Connect",
            Resource:
              "arn:aws:iot:" +
              Stack.of(this).region +
              ":" +
              Stack.of(this).account +
              ":client/${iot:ClientId}",
          },
          {
            Effect: "Allow",
            Action: "iot:Subscribe",
            Resource:
              "arn:aws:iot:" +
              Stack.of(this).region +
              ":" +
              Stack.of(this).account +
              ":topicfilter/${iot:ClientId}/tyhac/sub/presign",
          },
          {
            Effect: "Allow",
            Action: "iot:Receive",
            Resource:
              "arn:aws:iot:" +
              Stack.of(this).region +
              ":" +
              Stack.of(this).account +
              ":topic/${iot:ClientId}/tyhac/sub/presign",
          },
          {
            Effect: "Allow",
            Action: "iot:Publish",
            Resource:
              "arn:aws:iot:" +
              Stack.of(this).region +
              ":" +
              Stack.of(this).account +
              ":topic/${iot:ClientId}/tyhac/pub/presign",
          },
        ],
      },
    });

    // IoT Core - Rule - Lambda
    const iotRuleLambdaPreSign = new iot.CfnTopicRule(this, "iot-rule-sign", {
      ruleName: "tyhacLambdaPresignRule",
      topicRulePayload: {
        description: "tyhac rule requests signed url from lambda",
        sql: "SELECT * FROM 'tyhac/pub/presign'",
        actions: [
          {
            lambda: { functionArn: lambdaSignUrl.functionArn },
          },
        ],
      },
    });

    // IoT Core - Rule - Lambda
    const iotRuleLambdaStats = new iot.CfnTopicRule(this, "iot-rule-stats", {
      ruleName: "tyhacLambdaStatsRule",
      topicRulePayload: {
        description: "tyhac rule requests stats data from lambda and dynamodb",
        sql: "SELECT * FROM 'tyhac/pub/stats'",
        actions: [
          {
            lambda: { functionArn: lambdaStats.functionArn },
          },
        ],
      },
    });

    // IoT Core - Rule - Dynamo
    const iotRuleDynamo = new iot.CfnTopicRule(this, "iot-rule-dynamo", {
      ruleName: "tyhacDynamoStatus",
      topicRulePayload: {
        description: "tyhac rule to update dynamodb with IoT data",
        sql: "SELECT * FROM 'tyhac/pub/status'",
        actions: [
          {
            dynamoDBv2: {
              roleArn: iotRuleDynamoRole.roleArn,
              putItem: {
                tableName: dynamoIoTdb.tableName,
              },
            },
          },
        ],
      },
    });

    // IoT Core - IAM Role - Dynamo
    const iotRuleDynamoRole = new iam.Role(this, "iot-role", {
      assumedBy: new ServicePrincipal("iot.amazonaws.com"),
    });

    // Lambda - permissions - s3 put
    bucketRaw.grantPut(lambdaSignUrl);
    bucketRaw.grantReadWrite(lambdaStaging);
    bucketStaging.grantPut(lambdaStaging);
    bucketFinal.grantPut(lambdaStaging);
    bucketFinal.grantRead(lambdaPredictor);

    // DynamoDB - permissions
    dynamoIoTdb.grantReadData(lambdaStaging);
    dynamoIoTdb.grantReadData(lambdaPredictor);
    dynamoIoTdb.grantReadData(lambdaStats);

    // Lambda - permissions - IoT publish to MQTT
    // TODO: Refine scope
    lambdaSignUrl.addToRolePolicy(
      new iam.PolicyStatement({
        actions: ["iot:Publish"],
        resources: ["*"],
        // resources: [ "arn:aws:iot:" + Stack.of(this).region +  ":" + Stack.of(this).region + ":topic/tyhac/sub/presign" ]
      })
    );

    // Lambda - permissions - IoT publish to MQTT
    // TODO: Refine scope
    lambdaStaging.addToRolePolicy(
      new iam.PolicyStatement({
        actions: ["iot:Publish"],
        resources: ["*"],
        // resources: [ "arn:aws:iot:" + Stack.of(this).region +  ":" + Stack.of(this).region + ":topic/tyhac/sub/presign" ]
      })
    );

    // Lambda - permissions - IoT publish to MQTT
    // TODO: Refine scope
    lambdaStats.addToRolePolicy(
      new iam.PolicyStatement({
        actions: ["iot:Publish"],
        resources: ["*"],
        // resources: [ "arn:aws:iot:" + Stack.of(this).region +  ":" + Stack.of(this).region + ":topic/tyhac/sub/presign" ]
      })
    );

    // Lambda - permissions - IoT invoke lambda
    lambdaSignUrl.addPermission("iot-invocation", {
      principal: new iam.ServicePrincipal("iot.amazonaws.com"),
      sourceArn: iotRuleLambdaPreSign.attrArn,
    });
    // IoT Core - permissions - IoT write to dynamo status table
    iotRuleDynamoRole.addToPolicy(
      new iam.PolicyStatement({
        actions: ["dynamodb:PutItem"],
        resources: [dynamoIoTdb.tableArn],
      })
    );

    // Lambda - permissions - IoT invoke lambda
    lambdaStats.addPermission("iot-invocation", {
      principal: new iam.ServicePrincipal("iot.amazonaws.com"),
      sourceArn: iotRuleLambdaStats.attrArn,
    });

    // Lambda - permissions - IoT publish to MQTT
    // TODO: Refine scope
    lambdaPredictor.addToRolePolicy(
      new iam.PolicyStatement({
        actions: ["iot:Publish"],
        resources: ["*"],
        // resources: [ "arn:aws:iot:" + Stack.of(this).region +  ":" + Stack.of(this).region + ":topic/tyhac/sub/presign" ]
      })
    );

    // S3 Bucket event - fire when object created
    lambdaStaging.addEventSource(
      new S3EventSource(bucketRaw, {
        events: [s3.EventType.OBJECT_CREATED],
      })
    );

    // S3 Bucket event - fire when object created
    lambdaPredictor.addEventSource(
      new S3EventSource(bucketFinal, {
        events: [s3.EventType.OBJECT_CREATED],
      })
    );
  } // inner
} // outter
