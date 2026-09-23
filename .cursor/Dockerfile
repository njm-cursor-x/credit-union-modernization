FROM debian:bookworm-slim

# LameX Credit Union builds COBOL batch programs (GnuCOBOL) plus C++ teller and
# member-site binaries. Debian mirrors are reachable from Cloud Agent VMs, while
# the Ubuntu mirrors used by the default image are not, so this image installs
# the toolchain from deb.debian.org.
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        make \
        gnucobol \
        libncurses-dev \
        curl \
        ca-certificates \
        git \
    && rm -rf /var/lib/apt/lists/*

RUN useradd --create-home --shell /bin/bash ubuntu
USER ubuntu
