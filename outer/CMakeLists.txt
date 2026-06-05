cmake_minimum_required(VERSION 3.16)
project(scene_editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(OUTER_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(INNER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../inner")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

if (QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS OpenGL OpenGLWidgets)
endif()

add_executable(scene_editor
    ${OUTER_DIR}/source/main.cpp
    ${OUTER_DIR}/source/ProjectRoot.cpp
    ${OUTER_DIR}/source/MainWindow.cpp
    ${OUTER_DIR}/source/SceneFile.cpp
    ${OUTER_DIR}/source/PreviewWidget.cpp
    ${OUTER_DIR}/headers/MainWindow.h
    ${OUTER_DIR}/headers/PreviewWidget.h
    ${INNER_DIR}/source/object_factory.cpp
    ${INNER_DIR}/source/transform_wrapper.cpp
    ${INNER_DIR}/source/figures.cpp
    ${INNER_DIR}/source/manual_shapes.cpp
    ${INNER_DIR}/source/textures.cpp
    ${INNER_DIR}/source/textures_path.cpp
)

target_include_directories(scene_editor PRIVATE
    "${OUTER_DIR}/headers"
    "${INNER_DIR}"
    "${INNER_DIR}/headers"
    "${INNER_DIR}/stb"
)

target_compile_definitions(scene_editor PRIVATE GL_SILENCE_DEPRECATION)

if (QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(scene_editor PRIVATE
        Qt6::Widgets
        Qt6::OpenGLWidgets
    )
else()
    target_link_libraries(scene_editor PRIVATE Qt5::Widgets)
endif()

target_link_libraries(scene_editor PRIVATE GL GLU glut)
