# ndm-sys experiment container — M3 (wt-1-23)
# All ns-3 builds, unit tests, and scenario runs happen inside the container named `ndm-sys`.
# Base: Ubuntu 24.04 (GCC 13, CMake 3.28, ninja — satisfies ns-3.42 requirements: GCC >= 9,
# CMake >= 3.14).
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Core toolchain (ns-3.42 CMake build)
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        python3 python3-dev python3-pip \
        libtool autoconf automake \
    && rm -rf /var/lib/apt/lists/*

# Repo/transport + debugging
RUN apt-get update && apt-get install -y --no-install-recommends \
        git \
        openssh-client \
        rsync \
        ca-certificates curl wget \
        file less vim strace tcpdump \
        pkg-config \
        libssl-dev libxml2-dev \
        zsh \
    && rm -rf /var/lib/apt/lists/*

# Optional (uncomment when needed for a phase):
#   libgtk-3-dev     — ns-3 GUI / trace viewers
#   libclang-dev     — some ns-3 components
#   tmux             — long-running session hygiene inside the container

# NOTE: runs as root by design (research container, bind-mounted workspace,
# same convention as the existing IMEC containers).
WORKDIR /workspace
# Agents enter with:  docker exec -it ndm-sys zsh
CMD ["sleep", "infinity"]
