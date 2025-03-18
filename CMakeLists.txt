cmake_minimum_required(VERSION 3.10.0)
project(tulip-memory-pool VERSION 0.1.0 LANGUAGES C CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
add_compile_options(-Wall -O2)

# 查找pthread库
find_package(Threads REQUIRED)

# 设置目录
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INC_DIR ${CMAKE_SOURCE_DIR}/include)
set(TEST_DIR ${CMAKE_SOURCE_DIR}/tests)

# 源文件
file(GLOB SOURCES "${SRC_DIR}/*.cpp")

# 添加头文件目录
include_directories(${INC_DIR})

add_executable(main main.cpp)

# 创建单元测试可执行文件
# add_executable(unit_test 
#     ${SOURCES}
#     ${TEST_DIR}/UnitTest.cpp
# )

# # 创建性能测试可执行文件
# add_executable(perf_test
#     ${SOURCES}
#     ${TEST_DIR}/PerformanceTest.cpp
# )

# 链接pthread库
target_link_libraries(main PRIVATE Threads::Threads)
# target_link_libraries(unit_test PRIVATE Threads::Threads)
# target_link_libraries(perf_test PRIVATE Threads::Threads)


# # 添加测试命令
# add_custom_target(test
#     COMMAND ./unit_test
#     DEPENDS unit_test
# )

# add_custom_target(perf
#     COMMAND ./perf_test
#     DEPENDS perf_test
# )

# include(CTest)
# enable_testing()

# set(CPACK_PROJECT_NAME ${PROJECT_NAME})
# set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
# include(CPack)
