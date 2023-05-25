cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(libflv VERSION 0.1.0)

include(CTest)
enable_testing()



file(GLOB SourceFiles RELATIVE flv_app "*.cpp")


add_executable(flv_app main.cpp ${SourceFiles})

target_include_directories(flv_app PUBLIC
                           "${CMAKE_CURRENT_LIST_DIR}/libflv"
                           )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
