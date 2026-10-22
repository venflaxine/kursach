
cmake_minimum_required(VERSION 3.21)

project(LocalTextSimilarity
    VERSION 1.0.0
    DESCRIPTION "Локальный анализатор текстов"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_executable(local_antiplagiarism
    src/main.cpp
    src/main_window.cpp
    src/similarity_engine.cpp
    src/TextAnalyzer.cpp

    include/main_window.h
    include/similarity_engine.h
    include/TextAnalyzer.h
)

target_include_directories(local_antiplagiarism
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(local_antiplagiarism
    PRIVATE
        Qt6::Core
        Qt6::Widgets
)
