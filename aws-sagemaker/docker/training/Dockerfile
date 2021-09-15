ARG REGION=ap-southeast-2
ARG ARCH=$ARCH

# SageMaker PyTorch image
FROM 763104351884.dkr.ecr.$REGION.amazonaws.com/pytorch-training:1.8.1-$ARCH-py3

RUN apt-get -y update && apt-get install -y libsndfile1
RUN python -m pip --no-cache-dir install --upgrade matplotlib seaborn tqdm sagemaker-training ipywidgets
RUN python -m pip --no-cache-dir install torch==1.8.2+cu111 torchvision==0.9.2+cu111 torchaudio==0.8.2 -f https://download.pytorch.org/whl/lts/1.8/torch_lts.html
RUN python -m pip --no-cache-dir install fastaudio

ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8