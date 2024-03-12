# 在x64平台编译aarch64架构的eBPF程序

libbpf依赖libelf与libz

下载

```bash
# 下载libz
wget https://zlib.net/zlib-1.3.1.tar.gz
tar -xvf zlib-1。3.1.tar.gz
cd ./zlib-1.2.13/ && mkdir install
CC=riscv64-linux-gnu-gcc CXX=riscv64-linux-gnu-g++ ./configure --prefix=$PWD/install
make 
make install
cd ..
# 下载libelf
wget https://sourceware.org/elfutils/ftp/0.188/elfutils-0.188.tar.bz2
tar -xvf elfutils-0.188.tar.bz2
cd elfutils-0.188 && mkdir install
CC=riscv64-linux-gnu-gcc CXX=riscv64-linux-gnu-g++ LDFLAGS="-L$PWD/../zlib-1.2.13/install/lib" LIBS="-lz" ./configure --prefix=$PWD/install --build=x86_64-linux-gnu --host=riscv64-linux-gnu --disable-libdebuginfod --disable-debuginfod
make -j8
make -C libelf install



CC=/home/fzy/opt/GCC_ARM/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-gcc CXX=/home/fzy/opt/GCC_ARM/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-g++ LDFLAGS="-L$PWD/../zlib-1.3.1/install/lib" LIBS="-lz" ./configure --prefix=$PWD/install --build=x86_64-linux-gnu --host=aarch64-linux-gnu --disable-libdebuginfod --disable-debuginfod
```





```bash
# libelf下载地址: https://sourceware.org/elfutils/ftp/


```



参考文档

https://www.yezhem.com/index.php/archives/70/



https://github.com/libbpf/libbpf-bootstrap/issues/129



https://github.com/libbpf/libbpf-bootstrap/issues/144