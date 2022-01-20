cmake_minimum_required(VERSION 3.7)

project(test 
    LANGUAGES C CXX
    VERSION 0.1.0
)

add_subdirectory(lua)

add_executable(${PROJECT_NAME}
    main.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC lua
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE lua
)