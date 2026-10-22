cmake_minimum_required(VERSION 3.21)
project(LocalTextSimilarity VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.3 REQUIRED COMPONENTS Core Widgets)
qt_standard_project_setup()

qt_add_executable(local_antiplagiarism
    src/main.cpp
    src/main_window.cpp
    src/main_window.h
    src/similarity_engine.cpp
    src/similarity_engine.h
)

target_link_libraries(local_antiplagiarism PRIVATE
    Qt6::Core
    Qt6::Widgets
)

target_include_directories(local_antiplagiarism PRIVATE src)

set_target_properties(local_antiplagiarism PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.3 REQUIRED COMPONENTS Test)

    qt_add_executable(similarity_engine_tests
        tests/test_similarity_engine.cpp
        src/similarity_engine.cpp
        src/similarity_engine.h
    )

    target_link_libraries(similarity_engine_tests PRIVATE
        Qt6::Core
        Qt6::Test
    )
    target_include_directories(similarity_engine_tests PRIVATE src)

    add_test(NAME similarity_engine_tests COMMAND similarity_engine_tests)
endif()
