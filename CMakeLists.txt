cmake_minimum_required(VERSION 3.16)
project(TouristHelper CXX)

# mlpack 4 requires C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization is important for templates/header-only libs
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 1. Find Armadillo (The only thing you actually need to LINK)
find_package(Armadillo REQUIRED)

# 2. Find OpenMP (Recommended for speed)
find_package(OpenMP)

add_executable(TouristHelper main.cpp)

# 3. Include Directories
target_include_directories(TouristHelper PRIVATE
        ${ARMADILLO_INCLUDE_DIRS}
        # mlpack and ensmallen are usually in /usr/include on Arch,
        # so they are found automatically.
)

# 4. Link Libraries
# REMOVED: "mlpack" (It does not exist as a binary anymore)
target_link_libraries(TouristHelper PRIVATE
        ${ARMADILLO_LIBRARIES}
)

# Link OpenMP if found
if(OpenMP_CXX_FOUND)
    target_link_libraries(TouristHelper PRIVATE OpenMP::OpenMP_CXX)
endif()