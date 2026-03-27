cmake_minimum_required(VERSION 3.20)
project(mangadawg LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git
  GIT_TAG 1.11.2
)

FetchContent_Declare(
  akane
  GIT_REPOSITORY https://github.com/aimardcr/akane.git
  GIT_TAG master
)

FetchContent_MakeAvailable(cpr akane)

add_executable(mangadawg src/main.cpp)
target_include_directories(mangadawg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mangadawg PRIVATE cpr::cpr akane)
