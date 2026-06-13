cmake_minimum_required(VERSION 3.20)
project(coremetrics CXX)

# The Makefile is still the day-to-day build driver; this CMakeLists exists so
# IDEs and reviewers who expect CMake have a first-class entry point. Both
# build systems target the same C++23 sources and link the same SDL3 trio.

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(SDL3 IMPORTED_TARGET sdl3 sdl3-ttf sdl3-image)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)
if(APPLE)
    include_directories(/opt/homebrew/include)
    link_directories(/opt/homebrew/lib)
endif()

file(GLOB SRC_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tests/*.cpp")

add_executable(coremetrics ${CMAKE_SOURCE_DIR}/coremetrics.cpp ${SRC_SOURCES})
add_executable(tests ${TEST_SOURCES} ${SRC_SOURCES})

foreach(target coremetrics tests)
    if(APPLE)
        target_link_libraries(${target} PRIVATE SDL3 SDL3_ttf SDL3_image
                              "-framework IOKit" "-framework CoreFoundation")
    elseif(PkgConfig_FOUND AND SDL3_FOUND)
        target_link_libraries(${target} PRIVATE PkgConfig::SDL3)
    else()
        target_link_libraries(${target} PRIVATE SDL3 SDL3_ttf SDL3_image)
    endif()
endforeach()

option(COREMETRICS_ASAN "Build with AddressSanitizer" OFF)
option(COREMETRICS_UBSAN "Build with UndefinedBehaviorSanitizer" OFF)

if(COREMETRICS_ASAN)
    foreach(target coremetrics tests)
        target_compile_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
        target_link_options(${target} PRIVATE -fsanitize=address)
    endforeach()
endif()

if(COREMETRICS_UBSAN)
    foreach(target coremetrics tests)
        target_compile_options(${target} PRIVATE -fsanitize=undefined -fno-omit-frame-pointer -g)
        target_link_options(${target} PRIVATE -fsanitize=undefined)
    endforeach()
endif()
