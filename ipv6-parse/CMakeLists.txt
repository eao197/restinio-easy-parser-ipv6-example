project(ipv6-parse CXX)

find_package(restinio CONFIG REQUIRED)
find_package(doctest CONFIG REQUIRED)

set(UNITTEST ipv6-parse)
set(UNITTEST_SRCFILES main.cpp)

add_executable(${UNITTEST} ${UNITTEST_SRCFILES})

target_link_libraries(${UNITTEST} PRIVATE restinio::restinio)
target_link_libraries(${UNITTEST} PRIVATE doctest::doctest)
target_include_directories(${UNITTEST} PRIVATE ${CMAKE_SOURCE_DIR})

if (WIN32)
	target_link_libraries(${UNITTEST} PRIVATE wsock32 ws2_32)
endif ()

if (MSVC)
    target_compile_options(${UNITTEST} PRIVATE /bigobj)
endif ()

add_test(NAME ${UNITTEST} COMMAND ${UNITTEST})
