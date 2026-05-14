cmake_minimum_required(VERSION 3.16)
project(trpoKursach LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(${PROJECT_NAME}
    main.cpp
    mainwindow.cpp
    code_editor.cpp
    text_io.cpp
    diff_engine.cpp
    intraline_diff.cpp
    diff_text_delegate.cpp
    mainwindow.h
    code_editor.h
    text_io.h
    diff_engine.h
    diff_types.h
    intraline_diff.h
    diff_text_delegate.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)
