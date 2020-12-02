cmake_minimum_required(VERSION 3.14)
project(Expected CXX)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()

file(GLOB EXPECTED_HEADERS "include/*.hpp")
file(GLOB EXPECTED_SOURCES "src/*.cpp")
add_library(expected ${EXPECTED_SOURCES})

target_include_directories(expected
    PUBLIC
        $<INSTALL_INTERFACE:include>    
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if(${BUILD_SHARED_LIBS})
    target_compile_definitions(expected PRIVATE "EXPECTED_API=__EXPECTED_EXPORT")
else()
    target_compile_definitions(expected PUBLIC "EXPECTED_STATIC_DEFINE")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(expected PUBLIC "-fcoroutines")
endif()

if(WIN32)
    set(STACKTRACE "stacktrace_windbg")
else()
    set(STACKTRACE "stacktrace_basic")
endif()
find_package(Boost REQUIRED COMPONENTS ${STACKTRACE} REQUIRED)
target_link_libraries(expected PRIVATE Boost::${STACKTRACE})

option(EXPECTED_INSTALL "Install Expected library" OFF)
option(EXPECTED_BUILD_TESTS "Build tests." OFF)

if(${EXPECTED_INSTALL})
    install(FILES ${EXPECTED_HEADERS} DESTINATION include/linq)
    install(TARGETS expected EXPORT expected-targets)
    install(EXPORT expected-targets DESTINATION lib/cmake/expected)
    install(FILES cmake/expected-config.cmake DESTINATION lib/cmake/expected)
endif()

if(${EXPECTED_BUILD_TESTS})
    enable_testing()
    add_subdirectory(test)
endif()
