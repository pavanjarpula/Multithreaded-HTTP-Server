FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    ninja-build \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY CMakeLists.txt .
COPY include/ include/
COPY src/ src/

RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build

FROM ubuntu:22.04

WORKDIR /app

COPY --from=builder /app/build/http_server .
COPY public/ public/

EXPOSE 8080

ENV PORT=8080
ENV THREADS=4

CMD ./http_server --port $PORT --threads $THREADS
