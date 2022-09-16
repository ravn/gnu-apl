FROM ubuntu:latest as ubuntu-build-essential
RUN apt-get update
RUN apt-get install -y --no-install-recommends apt-utils build-essential sudo git psmisc

FROM ubuntu-build-essential as configured
COPY . .
RUN ./configure

FROM configured as compiled
RUN make -j 4

FROM compiled as tested
RUN make -C src test

