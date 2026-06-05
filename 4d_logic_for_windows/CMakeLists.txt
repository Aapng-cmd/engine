cmake_minimum_required(VERSION 3.15)
project(4D_Graphics_Engine CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Werror=return-type")

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

add_executable(graphics
    graphics.cpp
    Scene.cpp
    Camera.cpp
    Camera3D.cpp
    Camera4D.cpp
    Object.cpp
    Object3D.cpp
    Object4D.cpp
    utils.cpp
)

target_include_directories(graphics PRIVATE ${OPENGL_INCLUDE_DIR} ${GLUT_INCLUDE_DIRS})
target_link_libraries(graphics PRIVATE OpenGL::GL GLUT::GLUT)
