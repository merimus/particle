cmake_minimum_required(VERSION 3.5.0)
project(Particle VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(TBB REQUIRED)

add_executable(Particle benchmark.cpp geom.hpp bhqt.hpp)
target_link_libraries(${PROJECT_NAME} PUBLIC TBB::tbb)

