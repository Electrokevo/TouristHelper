cmake_minimum_required(VERSION 3.16)

# Recomendado para RTX 2060 (Compute Capability 7.5)
set(CMAKE_CUDA_ARCHITECTURES 75)

if(NOT DEFINED CMAKE_CUDA_COMPILER)
  set(CMAKE_CUDA_COMPILER "/opt/cuda/bin/nvcc")
endif()

if(NOT DEFINED CUDAToolkit_ROOT)
  set(CUDAToolkit_ROOT "/opt/cuda")
endif()

# Add CUDA to languages so CMake looks for nvcc immediately
project(TouristHelper LANGUAGES CXX CUDA)

# Use C++20 as requested in your file
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. Dependencies (Arch Repos) ---

# Find MLPack (Community Repo)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MLPACK REQUIRED mlpack)

# Find Armadillo (Extra Repo)
find_package(Armadillo REQUIRED)

# Find OpenMP (Core/Extra Repo)
find_package(OpenMP)

# --- 2. LibTorch (AUR) ---
# On Arch, the 'libtorch' or 'libtorch-cxx11-abi' AUR packages install to
# /opt/libtorch. We append this to the search path so find_package works
# automatically.

# REEMPLAZARLO. reemplazar entre entas lists
# list(APPEND CMAKE_PREFIX_PATH "/opt/libtorch")
# reemplazo con la linea de arriba para ejecutarlo con la gpu
list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/libs/libtorch")


find_package(Torch REQUIRED)

# --- 3. Build Configuration ---
add_definitions(-DMLPACK_ENABLE_ANN_SERIALIZATION)
# Note: Using the file names from our consolidated solution
add_executable(TouristHelper main.cpp Helpers.cpp Encoders.cpp)

# Include Directories
target_include_directories(
  TouristHelper
  PRIVATE ${MLPACK_INCLUDE_DIRS} ${ARMADILLO_INCLUDE_DIRS}
          # Torch includes are handled automatically by target_link_libraries
          # below
)

# Compile Options (Optimizations)
target_compile_options(TouristHelper PRIVATE ${MLPACK_CFLAGS_OTHER} -O3
                                             ${TORCH_CXX_FLAGS})

# Link Libraries
target_link_libraries(
  TouristHelper
  PRIVATE ${MLPACK_LIBRARIES} ${ARMADILLO_LIBRARIES}
          ${TORCH_LIBRARIES} # Links CUDA, CUDART, and LibTorch automatically
)

# OpenMP Linking
if(OpenMP_CXX_FOUND)
  target_link_libraries(TouristHelper PRIVATE OpenMP::OpenMP_CXX)
endif()
