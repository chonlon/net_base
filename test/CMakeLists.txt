include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

set(
	TESTS
	log_test.cpp
)


AddTest("net_base" "net_base" "./" ${TESTS})

set(
	BENCHMARKS
	file_write_speed.cpp
)

AddExeFromFilesWithLib("net_base" "net_base" "./" ${BENCHMARKS})