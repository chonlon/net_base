include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

set(
	TESTS
	log_test.cpp
)

configure_file("../bin/conf/test.yml" "${EXECUTABLE_OUTPUT_PATH}/conf/test.yml" COPYONLY)
configure_file("../bin/conf/test.json" "${EXECUTABLE_OUTPUT_PATH}/conf/test.json" COPYONLY)

AddTest("net_base" "net_base" "./" ${TESTS})

set(
	BENCHMARKS
	file_write_speed.cpp
)

AddExeFromFilesWithLib("net_base" "net_base" "./" ${BENCHMARKS})