FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    libreadline-dev \
    git \
    fuse3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN make clean && make

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libreadline8 \
    sudo \
    fdisk \
    util-linux \
    fuse3 \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m -s /bin/bash testuser
RUN useradd -m -s /bin/bash admin || true

COPY --from=builder /app/kubsh1 /usr/local/bin/kubsh1


ENV HOME=/root
ENV USER=root
ENV SHELL=/bin/bash
ENV PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

WORKDIR /root

RUN mkdir -p /users
CMD [ "bash", "-c", "kubsh1"]
