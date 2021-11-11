FROM ubuntu:focal

# setup build environment
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential git cmake pkg-config autoconf dh-autoreconf libglib2.0-dev flex bison libjson-c-dev libpixman-1-dev libcapstone-dev golang clang-format python-is-python3

# fetch sources
RUN git clone https://github.com/0xf4b1/bsod-kernel-fuzzing && cd bsod-kernel-fuzzing && git submodule init && git submodule update --depth=1 && cd kvm-vmi/kvm-vmi && git submodule init && git submodule update --depth=1 qemu

# build libkvmi
RUN cd /bsod-kernel-fuzzing/kvm-vmi/libkvmi && ./bootstrap && ./configure && make -j$(nproc) && make install

# build libvmi
RUN cd /bsod-kernel-fuzzing/kvm-vmi/libvmi && mkdir build && cd build && cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local -DENABLE_KVM=ON -DENABLE_XEN=OFF -DENABLE_BAREFLANK=OFF && make -j$(nproc) && make install && ldconfig

# build qemu
RUN cd /bsod-kernel-fuzzing/kvm-vmi/kvm-vmi/qemu && ./configure --target-list=x86_64-softmmu --prefix=/usr/local && make -j$(nproc) && make install

# build bsod-afl
RUN cd /bsod-kernel-fuzzing/bsod-afl/AFLplusplus && make -j$(nproc)
RUN cd /bsod-kernel-fuzzing/bsod-afl && mkdir build && cd build && cmake .. && make

# build bsod-syzkaller
RUN cd /bsod-kernel-fuzzing/bsod-syzkaller/syzkaller && make generate && make
RUN cd /bsod-kernel-fuzzing/bsod-syzkaller/syz-bp-cov && make

CMD ["/bin/bash"]