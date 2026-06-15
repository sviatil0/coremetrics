FROM mcr.microsoft.com/devcontainers/cpp:ubuntu-24.04
RUN apt-get update && apt-get install -y \
  libsdl3-dev libsdl3-ttf-dev libsdl3-image-dev \
  libfreetype-dev libharfbuzz-dev \
  && rm -rf /var/lib/apt/lists/*
