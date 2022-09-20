# Download base image ubuntu 18.04
FROM ubuntu:18.04

# LABEL about the custom image
LABEL maintainer="BTCU developers team"
LABEL version="0.1"
LABEL description="This is Docker Image for Building BTCU node with preinstalled packages."

# Disable Prompt During Packages Installation
ARG DEBIAN_FRONTEND=noninteractive

# Update Ubuntu Software repository
RUN apt update
RUN apt install unzip
RUN apt-get update && apt-get install -y wget

# Copy install_ubuntu.sh script and define default command for the container
COPY install_ubuntu_btcu_image.sh /install_ubuntu_btcu_image.sh
RUN bash -c "chmod 777 install_ubuntu_btcu_image.sh"
RUN bash -c "/install_ubuntu_btcu_image.sh"
