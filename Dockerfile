# Stage 1 — Build
FROM amazonlinux:2023 AS builder

RUN dnf update -y -q && \
    dnf install -y gcc-c++ cmake git openssl-devel tar gzip && \
    dnf clean all

# AWS SDK
RUN git clone --depth=1 --recurse-submodules --shallow-submodules \
        -b main https://github.com/aws/aws-sdk-cpp.git /tmp/aws-sdk-cpp && \
    mkdir -p /tmp/aws-sdk-cpp/build && cd /tmp/aws-sdk-cpp/build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DBUILD_ONLY="s3;dynamodb;sns" \
             -DENABLE_TESTING=OFF \
             -DAUTORUN_UNIT_TESTS=OFF \
             -DCMAKE_INSTALL_PREFIX=/usr/local && \
    make -j2 && make install && \
    rm -rf /tmp/aws-sdk-cpp

# StreamForge
WORKDIR /app
COPY . .
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DCMAKE_PREFIX_PATH=/usr/local && \
    make streamforge -j2

# Stage 2 — Runtime
FROM amazonlinux:2023

RUN dnf install -y openssl libstdc++ && dnf clean all

COPY --from=builder /usr/local/lib64/libaws-cpp-sdk-*.so* /usr/local/lib64/
COPY --from=builder /usr/local/lib64/libaws-crt-cpp.so*   /usr/local/lib64/
COPY --from=builder /usr/local/lib64/libaws-c-*.so*        /usr/local/lib64/
COPY --from=builder /app/build/streamforge                 /usr/local/bin/streamforge

RUN echo '/usr/local/lib64' > /etc/ld.so.conf.d/aws-sdk.conf && ldconfig

EXPOSE 8080 9090

CMD ["/usr/local/bin/streamforge"]
