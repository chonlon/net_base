### log speed

- ./log_speed.cpp
  
  使用stringstream来作为logger的缓冲:
  
  | name                                | 1                           | 2           | 3           | 4           | avg           |
  | ----------------------------------- | --------------------------- | ----------- | ----------- | ----------- | ------------- |
  | simple log                          | 47.767Mib/s \| 289519time/s | 48.129Mib/s | 47.179Mib/s | 47.739Mib/s | 47.7035Mib/s  |
  | protected log                       | 47.753Mib/s \| 289435time/s | 47.125Mib/s | 46.951Mib/s | 47.152Mib/s | 47.24525Mib/s |
  | str log                             | 49.412Mib/s \| 299490time/s | 49.694Mib/s | 49.001Mib/s | 48.769Mib/s | 49.219Mib/s   |
  | simple log without formatters       | 42.890Mib/s                 | 42.220Mib/s | 41.947Mib/s | 41.642Mib/s | 42.17475Mib/s |
  | log  without formatters and flusher | 45.498Mib/s                 | 45.118Mib/s | 45.160Mib/s | 44.683Mib/s | 45.11475Mib/s |

  使用自定义的LogSSteam来作为缓冲(主要优化是使用thread_local来缓存内存, 避免内存频繁申请释放):
  
  | name/time                          | 1      | 2      | 3      |
  | ---------------------------------- | ------ | ------ | ------ |
  | simple log                         | 68.377 | 71.213 | 66.973 |
  | protoected log                     | 73.625 | 73.625 | 69.691 |
  | str log                            | 77.252 | 76.743 | 77.730 |
  | simple log without formatters      | 66.128 | 68.455 | 63.954 |
  | log without formatters and flusher | 71.264 | 71.684 | 71.790 |

- 结论

  - **simple log without formatters** 和 **log  without formatters and flusher** 速度反而更低原因应该是长度不一致(去除formatter以后长度变短).
  - **log  without formatters and flusher** 中速度和**simple log**中相当, **log  without formatters and flusher** 去除了formatters和flushers, 说明瓶颈不在文件读写上, 反而在std::stringstream上.
  - 根据火焰图, flush占用的cpu只有7.7%, 主要占用cpu的是格式化日期(20%)以及LogWrapper部分使用的sstream(37%). 可采用的优化的主要是:

    1. 格式化日期可以结合到Timer中做, 做到ms级日志(代价是可能有延迟), 而不是每次格式化时间

    2. LogWrapper中的SStream也采用缓存来避免内存分配的开销


  
  
### ttcp speed

- ./ttcp_speed.cpp

- 81920 次, buffer size 1004

- run on lo interface

- 单线程

  

| name           | 1      | 2      | 3      | 4      | avg      |
| -------------- | ------ | ------ | ------ | ------ | -------- |
| sync sysapi    | 12.017 | 12.027 | 11.849 | 11.870 | 11.94075 |
| async fcontext | 11.672 | 11.704 | 12.010 | 11.988 | 11.8435  |
| async ucontext | 11.054 | 11.156 | 11.444 | 11.185 | 11.20975 |

*注: 单位Mib/s*

- 使用ucontext协程+epoll的性能约为阻塞的性能的 93.878%.
- 使用ucontext协程+epoll的性能约为fcontext的 94.648%