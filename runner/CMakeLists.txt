set(
    Runners
    runner_log.cpp
    runner_info.cpp
    runner_config.cpp
)

configure_file("./bin/conf/test.yml" "${EXECUTABLE_OUTPUT_PATH}/bin/conf/test.yml" COPYONLY)

AddExeFromFilesWithLib("net_base" "net_base" "./" ${Runners})