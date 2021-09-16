FROM public.ecr.aws/lambda/python:3.8

ENV NUMBA_CACHE_DIR=/tmp/

# Copy function code
COPY . ${LAMBDA_TASK_ROOT}

# Install the function's dependencies using file requirements.txt
# from your project folder.

RUN yum update -y && yum install -y libsndfile-devel libgomp1 gcc

COPY requirements.txt  .
RUN  pip3 install -r requirements.txt --target "${LAMBDA_TASK_ROOT}"

# Set the CMD to your handler (could also be done as a parameter override outside of the Dockerfile)
CMD [ "lambda.handler" ] 