FROM ubuntu:22.04

# set the timezone so it doesn't prompt
RUN apt-get update && DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get -y install tzdata

# TODO: Move these deps to Alpine
# TODO: Figure out the deps that River actually needs
# runtime requirements for Serenity and River
RUN apt-get update && apt-get install -yqq \
	iproute2 \
    libsnappy-dev \
    zlib1g-dev \
    libzstd-dev \
    libbz2-dev \
    liblz4-dev \
    libprocps-dev

# binary deps
COPY lib/ /usr/local/lib/
RUN ldconfig

# app files
COPY bin/ /river/

EXPOSE {{ESTATE_RIVER_LISTEN_PORT}}
ENTRYPOINT ["/river/{{ESTATE_RIVER_DAEMON}}"]