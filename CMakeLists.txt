cmake_minimum_required(VERSION 3.5.0)
project(Particle VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(TBB REQUIRED)

find_package(vsg 1.0.0 REQUIRED)

# find the optional vsgXchange that can be used for reading a range of image and 3d model formats and shader compilation
find_package(vsgXchange 1.0.0 QUIET)

add_executable(Particle benchmark.cpp geom.hpp bhqt.hpp)
target_link_libraries(${PROJECT_NAME} PUBLIC TBB::tbb)

add_executable(Main main.cpp geom.hpp bhqt.hpp)
target_link_libraries(Main vsg::vsg)

if (vsgXchange_FOUND)
    target_compile_definitions(Main PRIVATE vsgXchange_FOUND)
    target_link_libraries(Main vsgXchange::vsgXchange)
endif()
