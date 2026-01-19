cmake_minimum_required(VERSION 3.16)
project(ServerLookup LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. OPENCV
find_package(OpenCV REQUIRED)

# 2. THREADS (Para httplib)
find_package(Threads REQUIRED)

# 3. EJECUTABLE
add_executable(Server server_lookup.cpp)

# 4. LINKING
target_link_libraries(Server
    PRIVATE
    ${OpenCV_LIBS}
    Threads::Threads
)
