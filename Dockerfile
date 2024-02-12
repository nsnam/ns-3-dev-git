# Source : https://github.com/NotSpecial/ns-3-dev/blob/master/Dockerfile

FROM ubuntu:18.04

# Install dependencies
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -y \
    # Base requirements
    git gcc g++ python3 python3-pip \ 
    # Additional features
    libboost-all-dev libxml2-dev libgsl-dev \
    libgcrypt-dev libgtk-3-dev python3-dev \
    # Debugging and Formatting
    gdb valgrind clang-format \
    # Statistics framework
    sqlite sqlite3 libsqlite3-dev \
    # Packet inspection
    tcpdump wireshark && \ 
    # Cleanup
    rm -rf /var/lib/apt/lists/*

# Copy files and install waf helper script
COPY . /ns3
COPY ./waf-helper.sh /usr/local/bin/waf

# Configure and build
RUN waf configure --enable-examples --enable-tests && waf

# Change workdir
WORKDIR /ns3/scratch