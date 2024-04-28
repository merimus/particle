cmake_minimum_required(VERSION 3.5.0)
project(Particle VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#add_executable(Particle benchmark.cpp geom.hpp bhqt.hpp)
#target_link_libraries(${PROJECT_NAME} PUBLIC TBB::tbb)

set(CMAKE_CXX_FLAGS "-gdwarf-4 -Og")
add_executable(gltoy main.cpp)
set(GLEW_USE_STATIC_LIBS True)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)

find_package(glfw3 REQUIRED)
target_link_libraries(gltoy ${OPENGL_LIBRARIES} glm::glm glfw  GLEW::GLEW )
