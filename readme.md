## build
- deps
``` shell
 git clone https://github.com/jbeder/yaml-cpp.git
 mkdir build && cd build && make install

 git clone https://github.com/nlohmann/json.git
 mkdir build && cd build && make install

 git clone https://github.com/fmtlib/fmt.git
 mkdir build && cd build && make install

 sudo apt install  -y libboost-context-dev
```
- build & install
```
    $ mkdir build
    $ cd build
    $ cmake .. [-DEnableTests=off] [-DEnableRunner=off] [-DEnableStaticLibBuild=ON] [-DEnableDynamicLibBuild=ON]
    $ make
    $ make install
```

## example
    ./runner
## tests
    ./tests

## TODO
### logger
- 定时切换/定时flush
- logger 的stringstream复用或者使用专门设计的buffer, 避免频繁申请/释放内存降低性能
### 协程
- n:m协程模型
- work steal
- taskgroup/task抽象
### 定时器(optional)
- 使用timer_thread
### 网络
- http
    - 注意完整读取(\r\n).
    - 注意结合config设置超时.
- https
- iobuffer抽象
    - write时内核缓冲不够, 循环写
    - 指定大小循环读(粘包问题)
    - 避免智能指针来作为RAII临时方案
- (optional) RPC
### 内存
- (optional)alloctor
- 协程池
- try tcmalloc/jemalloc and bechmark
### SQL
- redis
- mysql