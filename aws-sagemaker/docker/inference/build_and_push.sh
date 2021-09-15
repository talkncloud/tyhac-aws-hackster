#!/usr/bin/env bash

# This script shows how to build the Docker image and push it to ECR to be ready for use
# by SageMaker.

# The argument to this script is the image name. This will be used as the image on the local
# machine and combined with the account and region to form the repository name for ECR.
IMAGE="tyhac-sagemaker-inference-fastai"

# parameters
FASTAI_VERSION="1.0"
PY_VERSION="py36"

# Get the account number associated with the current IAM credentials
account=$(aws sts get-caller-identity --query Account --output text)

if [ $? -ne 0 ]
then
    exit 255
fi

# Get the region defined in the current configuration (default to us-west-2 if none defined)
region=$(aws configure get region)
region=${region:-us-west-2}

# If the repository doesn't exist in ECR, create it.

aws ecr describe-repositories --repository-names "${IMAGE}" > /dev/null 2>&1

if [ $? -ne 0 ]
then
    aws ecr create-repository --repository-name "${IMAGE}" > /dev/null
fi

# Get the login command from ECR and execute it directly
echo $(aws ecr get-login-password --region ${region} |docker login --username AWS --password-stdin ${account}.dkr.ecr.${region}.amazonaws.com)

# Get the login command from ECR in order to pull down the SageMaker PyTorch image
# https://github.com/aws/deep-learning-containers/blob/master/available_images.md
echo $(aws ecr get-login-password --region ${region} |docker login --username AWS --password-stdin 763104351884.dkr.ecr.${region}.amazonaws.com)

# loop for each architecture (cpu & gpu)
# removed cpu
for arch in cpu
do  
    echo "Building image with arch=${arch}, region=${region}"
    TAG="${FASTAI_VERSION}-${arch}-${PY_VERSION}"
    FULLNAME="${account}.dkr.ecr.${region}.amazonaws.com/${IMAGE}:${TAG}"
    docker build -t ${IMAGE}:${TAG} --build-arg ARCH="$arch"  --build-arg REGION="${region}"  .
    docker tag ${IMAGE}:${TAG} ${FULLNAME}
    docker push ${FULLNAME}
done
