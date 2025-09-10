FROM nginx:1.29

ARG BACKEND_HOST
ARG BACKEND_PORT
ARG FRONTEND_HOST
ARG FRONTEND_PORT

RUN apt update && apt install -y python3 && apt clean

COPY ./generate-config.py /generate-config.py
RUN chmod +x /generate-config.py && /generate-config.py && rm /generate-config.py