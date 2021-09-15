## TYHAC (turn your head and cough)
TYHAC (pronounced *tieack*) is an IoT covid-19 audio thing that uses deep learning to determine if an audio cough is positive or negative for covid-19.

## Buying Mick a coffee
If you enjoyed this project and feel like shouting me a coffee, I'll happily accept but if thats not for you, I'd appreciate if you could please star or share this project for visibility to help others.

<a href="https://www.buymeacoffee.com/talkncloud" target="_blank"><img src="https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png" alt="Buy Me A Coffee" style="height: 41px !important;width: 174px !important;box-shadow: 0px 3px 2px 0px rgba(190, 190, 190, 0.5) !important;-webkit-box-shadow: 0px 3px 2px 0px rgba(190, 190, 190, 0.5) !important;" ></a>

## Disclaimer
Not a doctor. No medical professionals have reviewed or worked on this project. Seek medical advice from medical professionals. This is a prototype.

## Intended audience
This project is for builders, thinkers and those who can't stop tinkering. If you're into IoT, ML, AWS and/or tech, then read on. 

This project is a submission to the [hackster healthy spaces with challenge 2021](https://www.hackster.io/contests/Healthy-Spaces-with-AWS) which uses an M5 Stack Core2 AWS IoT EduKit, a small esp32 device with a whole bunch of capabilities.

While not a requirement to publish the project publically, I am doing so as an AWS Community Builder in the hopes that this work (while not perfect) may help others solve a challenge across AWS IoT, SageMaker machine / deep learning and also CDK.

## High level design


## Using this repo
Due to the size and complexity of the project, I have split it up into three distinct sections:

1.  aws-cdk
    this folder contains the [cloud development kit](https://docs.aws.amazon.com/cdk/latest/guide/home.html) infrastructure as code to deploy the AWS cloud services used. This will deploy everything from IoT, DynamoDB and Lambda functions for prediction etc. 

2.  aws-iot
    the adruino code used to run the M5 Core2 EduKit. 

3.  aws-sagemaker
    the juicy notebooks and scripts used to run data preparation, training and inference with sagemaker.

## CDK
I have used [projen](https://github.com/projen/projen) to create the project, great project, check it out. To deploy the stack do the following:

1. npm install -g projen
2. yarn install
3. authenticate to aws using the cli tools either creds, sso or magic
3. projen deploy

## IoT
The device code has been developed in arduino, I have used [platform.io](https://platformio.org/) to help develop the project. This is my first real attempt at using platform.io after switching from the arduino ide. It's good, use it.

## Sagemaker
You don't need to deploy anything in sagemaker to run this project. The sagemaker code is not part of CDK, I've supplied the notebooks and code for people to get a feel for how it works in sagemaker and hopefully build on.

For the deep learning ML fans out there, the model deployed in this project achieves the following:

run: tyhac-fastai-2021-09-15-06-59-58-639

<img src="aws-sagemaker/roc_curve.jpg" width="500">

<img src="aws-sagemaker/confusion_matrix.jpg" width="500">

Using the notebooks provided you can have a bit of look into audio properties of covid patients, here is a sample of the data prep splitting the imbalance of the dataset and showing what it looks like:

<img src="aws-sagemaker/ml_show_batch.png" width="700">

## Costs

## Getting help

## Contributing


