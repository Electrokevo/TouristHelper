cmake_minimum_required(VERSION 3.16)
project(TouristHelper CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Armadillo REQUIRED)

find_package(OpenMP)

add_executable(TouristHelper main.cpp)

target_include_directories(TouristHelper PRIVATE
        ${ARMADILLO_INCLUDE_DIRS}
)

target_link_libraries(TouristHelper PRIVATE
        ${ARMADILLO_LIBRARIES}
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(TouristHelper PRIVATE OpenMP::OpenMP_CXX)
endif()