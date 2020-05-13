cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_MODULE_PATH})

# Defaults to CMAKE_BUILD_TYPE=Debug
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

project(faststat LANGUAGES C CUDA)
find_package(CUDAToolkit)

set(FASTSTAT_CFLAGS -std=gnu11 -Wall -Wextra -pedantic -Wno-sign-compare -g3)
set(FASTSTAT_CFLAGS_DEBUG   ${FASTSTAT_CFLAGS} -O0)
set(FASTSTAT_CFLAGS_RELEASE ${FASTSTAT_CFLAGS} -O3 -march=native -DNDEBUG)

set(CMAKE_C_FLAGS "")
set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_RELEASE "")

add_executable(faststat main.c)
target_compile_options(faststat PUBLIC "$<$<CONFIG:DEBUG>:${FASTSTAT_CFLAGS_DEBUG}>")
target_compile_options(faststat PUBLIC "$<$<CONFIG:RELEASE>:${FASTSTAT_CFLAGS_RELEASE}>")
target_link_libraries(faststat rt CUDA::nvml)
