FROM --platform=linux/amd64 ubuntu:20.04 as builder

RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential automake pkg-config libtool gettext libz-dev libpcre2-dev libssl-dev libedit-dev clang

COPY . /repo
WORKDIR /repo
RUN autoreconf -if
RUN ./configure --prefix=/install
RUN make -j8
RUN make install
RUN clang++ /repo/mayhem/fuzz_utf8_encode.cpp -fsanitize=fuzzer,address -I /install/include/asteria/ /install/lib/libasteria.so -o /fuzz -std=c++17

FROM ubuntu:20.04 as package
RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y libedit-dev libssl-dev
COPY --from=builder /install /install
COPY --from=builder /fuzz /fuzz_utf8_encode
ENV LD_LIBRARY_PATH=/install/lib/
