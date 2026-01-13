cmake_minimum_required(VERSION 3.16)
project(TouristBackend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# MLPack y Armadillo
find_package(PkgConfig REQUIRED)
pkg_check_modules(MLPACK REQUIRED mlpack)
find_package(Armadillo REQUIRED)
find_package(OpenMP)

# Añadir ejecutable
add_executable(TouristBackend server.cpp Encoders.cpp Helpers.cpp)

# Includes e implementación de librerías header-only
target_include_directories(TouristBackend PRIVATE ${CMAKE_SOURCE_DIR}) 

# Linkeo
target_link_libraries(TouristBackend
    ${MLPACK_LIBRARIES}
    ${ARMADILLO_LIBRARIES}
    OpenMP::OpenMP_CXX
    # pthread es necesario para httplib en Linux
    pthread 
)
