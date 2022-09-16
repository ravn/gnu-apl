FROM dqneo/ubuntu-build-essential as configured
COPY . .
RUN ./configure

FROM configured as compiled
RUN make -j 4

FROM compiled as tested
RUN make -C src test

