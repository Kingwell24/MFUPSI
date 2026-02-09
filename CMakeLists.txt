cmake_minimum_required(VERSION 3.10)
project(MFUPSI_Performance_Test)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用优化（用于性能测试）
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 输出编译标志
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "CXX flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "CXX flags Release: ${CMAKE_CXX_FLAGS_RELEASE}")

# 源文件
set(SOURCES
    src/main.cpp
    src/protocol.cpp
)

# 头文件目录
set(INCLUDE_DIRS
    src/
)

# 创建可执行文件
add_executable(mfupsi_perf_test ${SOURCES})
target_include_directories(mfupsi_perf_test PRIVATE ${INCLUDE_DIRS})

# 链接数学库
target_link_libraries(mfupsi_perf_test m)

# 安装规则
install(TARGETS mfupsi_perf_test DESTINATION bin)

# 打印项目信息
message(STATUS "MFUPSI Performance Test Project")
message(STATUS "  Executable: mfupsi_perf_test")
message(STATUS "  Source files: ${SOURCES}")
message(STATUS "  Include dirs: ${INCLUDE_DIRS}")
