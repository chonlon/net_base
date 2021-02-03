set(
    Runners
    runner_log.cpp
    runner_info.cpp
    runner_config.cpp
)

configure_file("../bin/conf/test.yml" "${EXECUTABLE_OUTPUT_PATH}/conf/test.yml" COPYONLY)
configure_file("../bin/conf/test.json" "${EXECUTABLE_OUTPUT_PATH}/conf/test.json" COPYONLY)

AddExeFromFilesWithLib("net_base" "net_base" "./" ${Runners})