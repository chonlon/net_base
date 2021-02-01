set(
    Runners
    runner_log.cpp
    runner_info.cpp
)

AddExeFromFilesWithLib("net_base" "net_base" "./" ${Runners})