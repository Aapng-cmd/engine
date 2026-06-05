cmake_minimum_required(VERSION 3.16)
project(scene_editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(INNER "${CMAKE_CURRENT_SOURCE_DIR}/../inner")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

if (QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS OpenGL OpenGLWidgets)
endif()

add_executable(scene_editor
    main.cpp
    ProjectRoot.cpp
    MainWindow.cpp
    SceneFile.cpp
    PreviewWidget.cpp
    ProjectRoot.h
    ${INNER}/object_factory.cpp
    ${INNER}/transform_wrapper.cpp
    ${INNER}/figures.cpp
    ${INNER}/manual_shapes.cpp
    ${INNER}/textures.cpp
    ${INNER}/textures_path.cpp
)

target_include_directories(scene_editor PRIVATE "${INNER}" "${INNER}/stb")

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
