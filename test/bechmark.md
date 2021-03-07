### log speed

- ./log_speed.cpp

  | name                                | 1                           | 2           | 3           | 4           | avg           |
  | ----------------------------------- | --------------------------- | ----------- | ----------- | ----------- | ------------- |
  | simple log                          | 47.767Mib/s \| 289519time/s | 48.129Mib/s | 47.179Mib/s | 47.739Mib/s | 47.7035Mib/s  |
  | protected log                       | 47.753Mib/s \| 289435time/s | 47.125Mib/s | 46.951Mib/s | 47.152Mib/s | 47.24525Mib/s |
  | str log                             | 49.412Mib/s \| 299490time/s | 49.694Mib/s | 49.001Mib/s | 48.769Mib/s | 49.219Mib/s   |
  | simple log without formatters       | 42.890Mib/s                 | 42.220Mib/s | 41.947Mib/s | 41.642Mib/s | 42.17475Mib/s |
  | log  without formatters and flusher | 45.498Mib/s                 | 45.118Mib/s | 45.160Mib/s | 44.683Mib/s | 45.11475Mib/s |
- 结论

  - **simple log without formatters** 和 **log  without formatters and flusher** 速度反而更低原因应该是长度不一致(去除formatter以后长度变短).
  - **log  without formatters and flusher** 中速度和**simple log**中相当, **log  without formatters and flusher** 去除了formatters和flushers, 说明瓶颈不在文件读写上, 反而在std::stringstream上.

### ttcp speed

- ./ttcp_speed.cpp

- 81920 次, buffer size 1004

- run on lo interface

- 单线程

  

| name           | 1      | 2      | 3      | 4      | avg      |
| -------------- | ------ | ------ | ------ | ------ | -------- |
| sync sysapi    | 12.017 | 12.027 | 11.849 | 11.870 | 11.94075 |
| async ucontext | 11.054 | 11.156 | 11.444 | 11.185 | 11.20975 |

*注: 单位Mib/s*

- 使用ucontext协程+epoll的性能约为阻塞的性能的 93.878%.