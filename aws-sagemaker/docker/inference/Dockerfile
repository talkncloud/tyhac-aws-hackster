ARG REGION=ap-southeast-2
ARG ARCH=$ARCH

# SageMaker PyTorch image
FROM 763104351884.dkr.ecr.$REGION.amazonaws.com/pytorch-inference:1.8.1-$ARCH-py3

RUN apt-get -y update && apt-get install -y libsndfile1
RUN python -m pip --no-cache-dir install --upgrade light-the-torch boto3
RUN ltt install fastaudio

ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8