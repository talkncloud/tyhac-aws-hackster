# The notes area
This is a document for me to throw some notes in, links etc and track it via the repo.

# Terms
A bunch of terms to remember:

f1
confusion matrix
weights
transfer learning
tensor
class
label
epoch

# Hard notes
- Everyone is using librosa
- Resnet50 deep learning model is mostly used in this type of task
- Sagemaker studio - gitignore large file sets, it hangs otherwise
- Hyperparameter training jobs / optimization. This seems great for people like me. Cost though
- Sagemaker exp and trials provide easy info in the studio, little config but seems useful
- SM prediction endpoints have design requirements
- FastAI need to bring your container more so with fastaudio
- Accuracy does not mean its good, especially in our case being binary
- SM Studio will eat your budget - watch out for EFS, close those sessions!

# Data sets
https://www.kaggle.com/himanshu007121/coughclassifier-trial
https://www.covid-19-sounds.org/en/blog/data_sharing.html
 - Need institution to sign on to obtain, it's also the fixed set at the date of the paper publication

# Reading
f1 scores to confusion matrix etc, i'll need this when the model is "working":
https://machinelearningmastery.com/classification-accuracy-is-not-enough-more-performance-measures-you-can-use/

https://aws.amazon.com/blogs/machine-learning/multimodal-deep-learning-approach-for-event-detection-in-sports-using-amazon-sagemaker/
- MobileNet

https://keras.io/api/applications/#usage-examples-for-image-classification-models

https://devopstar.com/2020/04/13/dog-bark-detector-machine-learning-model
- sir barks alot
- ml capstone has good descriptions - according to the comment?
- he's got some CDK - https://github.com/t04glovern/dog-bark-detection
- sqs q, ddb
https://mikesmales.medium.com/sound-classification-using-deep-learning-8bc2aa1990b7

https://cosminsanda.com/posts/voice-recognition-with-mxnet-and-sagemaker/
- txt to imae

https://github.com/giulbia/baby_cry_detection
- baby cry

https://github.com/aws-samples/amazon-sagemaker-audio-classification-pytorch

south africa:
- https://arxiv.org/pdf/2012.01926.pdf
- MFCC, CNN - image classification refs, good results
- Note the secondary dataset was 70% so no so good, i wonder if i'll get the same
- Hyperparameters in the table didn't make a lot of sense to me

New:
- https://arxiv.org/pdf/2004.01275.pdf
- Table of diases that cause coughs, interesting
- Physicians use cough sounds to diagnose table 1
- No feasible to train ai for coughs
- They use deep domain knowledge - MD's, ct scans, other data etc
- Some patients got phemenioa, is that different, how do you separate it? CT scans etc show that there is likely to be something different in the cough signature
- Ref to widely used studies for identify other coughs, note 80% was the confidence
- They use a pre-screening cough detector - supports my design
- fixed 3 second length
  - this kinda makes sense now, maybe take the median of all clips, find the min and use that instead though?
- Mel pitch catgeorization - good explanation of the melspec
- Cepstroal Coefficent - MFCC - i've seen this mentioned, this is done on the mel spec?
- 4 class of t-sne
- 3 parallel classifiers
- Tri-pronged mediator - interesting, all three or inconclusive

MIT:
- pdf on google drive (ieee)
- MFCC
- 3 parallel pre-trained model with Resnet50
- CNN based model
- forced-cough approach?
- orthangal audio biomarkers - expand into these areas, i agree, the value is for this immediate problem but outside of this also
- Hundreds of thousands of coughs - 5k positive c19
- Open Voice Brain Model Framework
- Three coughs per subject
- samples were saved withoutcompression in WAV format (16kbs bit-rate, single channel,opus codec
- Split into 6 second chunks and padded if needed ** interesting
- Then MFCC package on the chunks
- Lung diseases have distinct biomarkers - MJ? that would be interesting?
- Trained Resnet50 with input shape (is this the shape file?)
- They could detect origin and gender? English v Spanish, male v female
- 45% of c19 have a fever

More Notes:
-  For time i don't think a multi pronged approach is doable
- I need to flesh out the cough detection and how i'm saying that is reliable, high level
- The use of architecting is heavy in "New" research, i like this as i can lean on my skills in this area
- double check MIT 6second v 3 second
- double check MIT sentiment, i don't get why?

https://acoustics.asn.au/conference_proceedings/AAS2018/papers/p134.pdf
Bird Call Recognition - Resnet50

https://github.com/chen0040/keras-audio
- Interesting keras

https://www.kaggle.com/souro12/training-audio-sequence-using-resnet50
- Resnet50
- Respiratory Sound Database
- https://www.kaggle.com/vbookshelf/respiratory-sound-database

https://towardsdatascience.com/coronavirus-using-machine-learning-to-triage-covid-19-patients-980e62489fd4
- nice write up
- librosa again
- mel specs again but not spectrograms?
- more features, i'm noting this a few times now, i don't see that in papers though?
- good explainations though
- this is a binary classification problem, either covid or not, nice
- 

https://blog.betomorrow.com/keras-in-the-cloud-with-amazon-sagemaker-67cf11fb536
- Note overview of the lifecycle of sagemaker
- An organized folder structure will automatically generate matching labels
-- Should be organize into positive / negative / untested? 
- There are hard coded function names and params to be aware of

https://www.kaggle.com/tarunpaparaju/birdcall-identification-spectrogram-resnet
- Overview of resnet

https://www.analyticsvidhya.com/blog/2021/06/how-to-detect-covid19-cough-from-mel-spectrogram-using-convolutional-neural-network/
- Haven't seen anything this close before

http://cs230.stanford.edu/projects_winter_2021/reports/70560706.pdf
- explains how the virufy models works at a high level
- explains the images and the reason for mfcc in the network

https://docs.aws.amazon.com/machine-learning/latest/dg/binary-model-insights.html
- Amazon ML, WTF?
- Binary and AUC

https://aws.amazon.com/blogs/machine-learning/create-a-model-for-predicting-orthopedic-pathology-using-amazon-sagemaker/
- AUC in SageMaker

https://www.section.io/engineering-education/image-classifier-keras/
- single image x-ray predict

https://machinelearningmastery.com/tactics-to-combat-imbalanced-classes-in-your-machine-learning-dataset/
- Class imbalance