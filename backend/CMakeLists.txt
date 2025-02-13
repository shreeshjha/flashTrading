cmake_minimum_required(VERSION 3.10)
project(flashTrading LANGUAGES Fortran CXX)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)  # Crow needs threading support

# Download Crow if not present
include(FetchContent)
FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.0
)
FetchContent_MakeAvailable(Crow)

# Specify source files
set(FORTRAN_SOURCES
    order_book.f90
)

set(CPP_SOURCES
    trading_engine.cpp 
    server.cpp 
)

# Create the executable
add_executable(simulator ${FORTRAN_SOURCES} ${CPP_SOURCES})

# Link libraries
target_link_libraries(simulator PRIVATE 
    Crow::Crow 
    Threads::Threads
)

# Include directories
target_include_directories(simulator PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
