FROM ubuntu:22.04

ENV TZ=UTC
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    libspdlog-dev \
    nlohmann-json3-dev \
    libgflags-dev \
    libyaml-cpp-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN rm -rf build && \
    mkdir build && \
    cd build && \
    cmake -DFOXHTTP_BUILD_EXAMPLES=ON .. && \
    make -j$(nproc)

ENV LD_LIBRARY_PATH=/app/build