# Download prepared ubuntu 18.04 image
FROM  dim4egster/btculinuxtest:latest

WORKDIR /root
# LABEL about the custom image
LABEL maintainer="BTCU developers team"
LABEL version="0.1"
LABEL description="This is Docker Image for BTCU node."

# Copy build_node_docker.sh script and define default command for the container
COPY build_node_docker.sh /build_node_docker.sh
RUN bash -c "/build_node_docker.sh"

#RUN wget https://btcu.io/releases/chainstate_orion.zip
#RUN mkdir /root/.btcu
#RUN unzip -d /root/.btcu chainstate_orion.zip
ENTRYPOINT ["./orion/bin/btcud"]

# Expose Port for the Application
EXPOSE 3668 5668
