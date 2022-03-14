FROM ubuntu:21.04
MAINTAINER RedYre on Github
ENV DEBIAN_FRONTEND=noninteractive
ENV DOCKER_BUILDKIT=0
RUN apt-get update \
  && apt-get install -y git \
                        g++ \
                        make \
                        wget \
                        curl \
                        make \
                        gcc mono-mcs \
                        vim \
                        zip \
                        cmake \
                        libboost-dev \
                        libgtest-dev \
                        libboost-all-dev \
                        libcurl4-gnutls-dev \
                        libosmium-dev \
                        build-essential \
                        libsparsehash-dev \
                        --no-install-recommends apt-utils

COPY . /container
WORKDIR /container/bin
RUN cmake  -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC_LIBS=ON ..
RUN make -j2


# inspect the container 
CMD ["/bin/bash"]

# ./otter -osmpath ../mountme/bremen-latest.osm.pbf -ttlpath ../mountme/bremen.gz