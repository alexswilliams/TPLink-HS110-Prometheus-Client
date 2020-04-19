#!/usr/bin/env bash

# needs buildx enabled and a docker backend running
# e.g. with docker buildx create --use

docker buildx build . --platform=linux/arm/v6,linux/amd64 -t alexswilliams/tplink-hs110-prometheus-client:latest --push
