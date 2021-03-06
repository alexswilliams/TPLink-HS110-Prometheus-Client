FROM alpine:3.12.0 as build
# Updated here: https://hub.docker.com/r/arm32v6/alpine/tags

ARG CJSON_VERSION=1.7.13
# Updated here: https://github.com/DaveGamble/cJSON/releases


RUN apk --no-cache add build-base cmake wget tar binutils
RUN mkdir -p /build/cjson && \
    cd /build/cjson && \
    wget https://github.com/DaveGamble/cJSON/archive/v${CJSON_VERSION}.tar.gz -O cjson.tar.gz && \
    tar xf cjson.tar.gz && \
    cd cJSON-${CJSON_VERSION} && \
    mkdir cmake-build && \
    cd cmake-build && \
    cmake -DBUILD_SHARED_AND_STATIC_LIBS=On .. && \
    make VERBOSE=1 && \
    make install

COPY . /build/app
RUN cd /build/app && \
    mkdir cmake-build && \
    cd cmake-build && \
    cmake -v -DBUILD_STATIC=On .. && \
    make VERBOSE=1 && \
    strip tplink-hs110-client && \
    ls -lah tplink-hs110-client && \
    file tplink-hs110-client && \
    ldd ./tplink-hs110-client || true




FROM scratch
COPY --from=build /build/app/cmake-build/tplink-hs110-client /app/

CMD [ "/app/tplink-hs110-client" ]
