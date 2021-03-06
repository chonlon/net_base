include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

set(
	TESTS
	log_test.cpp
	config_test.cpp
	coroutine_test.cpp
	scheduler_test.cpp
)

configure_file("../bin/conf/test.yml" "${EXECUTABLE_OUTPUT_PATH}/conf/test.yml" COPYONLY)
configure_file("../bin/conf/test.json" "${EXECUTABLE_OUTPUT_PATH}/conf/test.json" COPYONLY)
configure_file("../bin/conf/main.json" "${PROJECT_BINARY_DIR}/test/conf/main.json" COPYONLY) #cmake 的gtest分析需要先执行, 所以需要一份config的拷贝.

AddTest("net_base" "net_base" "./" ${TESTS})

set(
	BENCHMARKS
	file_write_speed.cpp
	log_speed.cpp
)

AddExeFromFilesWithLib("net_base" "net_base" "./" ${BENCHMARKS})