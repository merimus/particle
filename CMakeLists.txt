cmake_minimum_required(VERSION 3.5.0)
project(Particle VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

FILE(GLOB imguiSrcs imgui/*.cpp)
include_directories(imgui imgui/backends)

set(GLEW_USE_STATIC_LIBS True)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(TBB REQUIRED)

add_executable(gltoy main.cpp ${imguiSrcs} imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl3.cpp)

target_link_libraries(gltoy ${OPENGL_LIBRARIES} glm::glm glfw  GLEW::GLEW )

add_executable(Particle newbench.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC glm::glm TBB::tbb)
