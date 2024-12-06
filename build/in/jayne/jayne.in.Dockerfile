FROM ubuntu:22.04

COPY bin/ /Jayne/
COPY lib/ /usr/local/lib/
RUN ldconfig

# TODO: remove the deps that the Serenity Client doesn't need
RUN apt-get update && apt-get install -yqq \
	libsnappy-dev \
    liblz4-dev \
    libz-dev \
    libzstd-dev \
    libbz2-dev \
    dotnet6 \
    ca-certificates

ENV ASPNETCORE_URLS=http://+:{{ESTATE_JAYNE_LISTEN_PORT}}
EXPOSE {{ESTATE_JAYNE_LISTEN_PORT}}
ENV DOTNET_ROOT=/usr/lib/dotnet
ENTRYPOINT ["/Jayne/Estate.Jayne"]
