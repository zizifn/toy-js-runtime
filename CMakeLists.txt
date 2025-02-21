cmake_minimum_required (VERSION 3.10)

project (toyjsruntime C)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g -DDEBUG")

# Copy a file from source to destination
file(COPY ${CMAKE_SOURCE_DIR}/src/example/libtest_lib.so DESTINATION ${CMAKE_BINARY_DIR})


# file(GLOB MODULE_SRC_FILES 
#     "${CMAKE_SOURCE_DIR}/src/module*.c"
# )

# libs

## ffi
find_library(FFI_LIB NAMES libffi ffi REQUIRED)

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
add_executable(toyjsruntime ${CMAKE_SOURCE_DIR}/src/main.c ${CMAKE_SOURCE_DIR}/src/module_quickjs_ffi.c)
# link 相关 lib，这里用qjs，因为 deps/quickjs CMakeLists.txt 输出结果是 qjs
target_link_libraries(toyjsruntime PRIVATE m qjs uv ${FFI_LIB})

add_executable(toyjsruntimec ${CMAKE_SOURCE_DIR}/src/toyjsruntimec.c)
target_link_libraries(toyjsruntimec PRIVATE m qjs)