FROM quay.io/pypa/manylinux_2_28_x86_64

RUN yum -y install rdma-core-devel libuv-devel

# The cuda is not available from dnf sources and have to download and install manually
RUN curl -O https://developer.download.nvidia.com/compute/cuda/12.8.0/local_installers/cuda-repo-rhel8-12-8-local-12.8.0_570.86.10-1.x86_64.rpm
RUN rpm -i cuda-repo-rhel8-12-8-local-12.8.0_570.86.10-1.x86_64.rpm
RUN dnf clean all
RUN dnf makecache

# install cuda toolkit
RUN dnf -y install cuda-toolkit-12-8

# build spdlog
WORKDIR /tmp
RUN git clone --branch v1.15.1 --recurse-submodules https://github.com/gabime/spdlog.git
WORKDIR /tmp/spdlog
RUN mkdir build
WORKDIR /tmp/spdlog/build
RUN cmake ..
RUN make install
RUN rm -rf /tmp/spdlog

# build fmt
WORKDIR /tmp
RUN git clone --branch 11.1.3 https://github.com/fmtlib/fmt.git
WORKDIR /tmp/fmt
RUN mkdir build
WORKDIR /tmp/fmt/build
RUN cmake ..
RUN make install
RUN rm -rf /tmp/fmt

# build flatbuffer
WORKDIR /tmp
RUN git clone --branch v25.2.10 https://github.com/google/flatbuffers.git
WORKDIR /tmp/flatbuffers
RUN cmake -G "Unix Makefiles" && \
    make && \
    make install

# Set up environment variables for CUDA (optional if you plan to use CUDA)
ENV PATH=/usr/local/flatbuffers/bin:$PATH
RUN rm -rf /tmp/flatbuffers

# Install boost
RUN dnf install -y boost boost-devel

# Install pybind11 for different versions of built-in python3 by almalinux
RUN /opt/python/cp310-cp310/bin/pip3 install pybind11
RUN /opt/python/cp311-cp311/bin/pip3 install pybind11

# In almalinux, setuptools for python3.12 is not installed
# so install it
RUN /opt/python/cp312-cp312/bin/pip3 install setuptools
RUN /opt/python/cp312-cp312/bin/pip3 install pybind11

# The above get the build environment ready!

# Optional: Define an entry point to run the executable directly
# ENTRYPOINT ["/app/build/my_executable"]
