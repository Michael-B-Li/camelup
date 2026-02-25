cmake_minimum_required(VERSION 3.20)
project(camelup LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(CAMELUP_BUILD_UI "Build optional terminal UI viewer" ON)

add_library(camelup_engine
    src/engine.cpp
)

target_include_directories(camelup_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(camelup
    src/main.cpp
)
target_link_libraries(camelup PRIVATE camelup_engine)

if(CAMELUP_BUILD_UI)
    add_executable(camelup_ui
        src/ui_main.cpp
    )
    target_link_libraries(camelup_ui PRIVATE camelup_engine)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(camelup_tests
        tests/engine_tests.cpp
    )
    target_link_libraries(camelup_tests PRIVATE camelup_engine)
    add_test(NAME camelup_tests COMMAND camelup_tests)
endif()
