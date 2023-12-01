cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0077 NEW)

set(VCPKG_FEATURE_FLAGS versions)

project(restinio-easy-parser-ipv6-example CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if ("GNU" STREQUAL CMAKE_CXX_COMPILER_ID)
	add_compile_options(-Wall -Wextra -pedantic)

	# eao197: uncomment when debug info has to be added to the binaries
	# even in release mode.
	# add_compile_options(-g)
	# add_link_options(-g)
endif()

if ("MSVC" STREQUAL CMAKE_CXX_COMPILER_ID)
	add_compile_options(/permissive-)
	add_compile_options(/utf-8)
endif()

enable_testing()

add_subdirectory(ipv6-parse)

