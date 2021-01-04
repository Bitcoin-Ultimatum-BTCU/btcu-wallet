# This Dockerfile script is a proof that Aleth can be built on CentOS.

FROM centos:7

RUN yum -q -y install git centos-release-scl
RUN yum -q -y install devtoolset-7-gcc-c++
RUN git clone --recursive https://github.com/ethereum/cpp-ethereum --single-branch --depth 100
RUN /cpp-ethereum/scripts/install_cmake.sh --prefix /usr

RUN source scl_source enable devtoolset-7 && cd /tmp && cmake /cpp-ethereum
RUN cd /tmp && make -j8 && make install

ENTRYPOINT ["/usr/local/bin/cpp-ethereum"]
