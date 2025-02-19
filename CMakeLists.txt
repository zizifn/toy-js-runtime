cmake_minimum_required (VERSION 3.10)

project (toyjsruntime C)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g -DDEBUG")


# libs
## quickjs
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(QUICKJS_DIR ${CMAKE_SOURCE_DIR}/deps/quickjs)
# 一定要把这个打开，要不然没有 QJS 的额外带的标准库
set(BUILD_QJS_LIBC ON)
add_subdirectory(${QUICKJS_DIR} ${CMAKE_BINARY_DIR}/quickjs)

# Add libuv submodule
set(LIBUV_DIR ${CMAKE_SOURCE_DIR}/deps/libuv)
# Include libuv as a subdirectory
add_subdirectory(${LIBUV_DIR} ${CMAKE_BINARY_DIR}/libuv)

# 生成 toy js runtime 的执行文件
add_executable(toyjsruntime ${CMAKE_SOURCE_DIR}/src/main.c)
# link 相关 lib，这里用qjs，因为 deps/quickjs CMakeLists.txt 输出结果是 qjs
target_link_libraries(toyjsruntime PRIVATE m qjs uv)