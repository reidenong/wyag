cmake_minimum_required(VERSION 3.16)
project(wyag LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ZLIB REQUIRED)

file(GLOB WYAG_SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(wyag ${WYAG_SOURCES})
target_include_directories(wyag PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(wyag PRIVATE ZLIB::ZLIB)
