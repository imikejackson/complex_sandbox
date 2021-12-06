file(TO_CMAKE_PATH "${sandbox_SOURCE_DIR}" sandbox_SOURCE_DIR_NORM)
file(TO_CMAKE_PATH "${sandbox_BINARY_DIR}" sandbox_BINARY_DIR_NORM)

set(SANDBOX_TEST_DIRS_HEADER ${sandbox_BINARY_DIR}/sandbox_test_dirs.h)

configure_file("${sandbox_SOURCE_DIR}/sandbox/sandbox_test_dirs.h.in"
        ${SANDBOX_TEST_DIRS_HEADER}
        @ONLY)


add_executable(sandbox ${sandbox_SOURCE_DIR}/sandbox/sandbox.cpp ${SANDBOX_TEST_DIRS_HEADER})

target_include_directories(sandbox PUBLIC ${ComplexCore_SOURCE_DIR}/src)

target_link_libraries(sandbox complex::complex complex::ComplexCore)

target_include_directories(sandbox PRIVATE ${sandbox_BINARY_DIR})


add_executable(datastructure_test ${sandbox_SOURCE_DIR}/sandbox/datastructure_test.cpp ${SANDBOX_TEST_DIRS_HEADER})
target_include_directories(datastructure_test PUBLIC ${ComplexCore_SOURCE_DIR}/src)
target_link_libraries(datastructure_test complex::complex complex::ComplexCore)
target_include_directories(datastructure_test PRIVATE ${sandbox_BINARY_DIR})
