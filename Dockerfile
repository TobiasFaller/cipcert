FROM ubuntu:24.04

RUN \
  apt-get update && \
  apt-get install -y --no-install-recommends cmake make g++ git zlib1g-dev ca-certificates && \
  apt-get clean

COPY . /cipcert
WORKDIR /cipcert
RUN \
  rm -rf build && \
  cmake -DCMAKE_BUILD_TYPE=Release -DTOOLS=ON -B build && \
  make -j$(nproc) -C build

ENTRYPOINT ["/cipcert/build/bin/check"]
