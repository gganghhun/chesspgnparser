FROM pytorch/pytorch:2.1.0-cuda11.8-cudnn8-runtime

WORKDIR /app

COPY train.py .
ENTRYPOINT ["python", "train.py"]