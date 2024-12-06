FROM ubuntu:22.04

# set the timezone so it doesn't prompt
RUN apt-get update && DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get -y install tzdata

# TODO: Move these deps to Alpine
# runtime requirements for Serenity and River
# TODO: Only install the requirements that Serenity actually needs
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
COPY bin/ /serenity/

EXPOSE {{ESTATE_SERENITY_GET_WORKER_PROCESS_ENDPOINT_PORT}}
EXPOSE {{ESTATE_WORKER_PROCESS_PORT_START}}-{{ESTATE_WORKER_PROCESS_PORT_END}}
ENTRYPOINT ["/serenity/{{ESTATE_SERENITY_DAEMON}}"]