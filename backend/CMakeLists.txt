cmake_minimum_required(VERSION 3.10)
project(flashTrading LANGUAGES Fortran CXX)

# Set C++ Standard to C++17 (Crow uses std::filesystem)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(Lua REQUIRED)
# (Assume QuickFIX is installed; adjust as needed.)

include(FetchContent)

FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.0
)
FetchContent_MakeAvailable(Crow)

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-18-1
)
FetchContent_MakeAvailable(asio)

# Fortran sources
set(FORTRAN_SOURCES
    order_book.f90
)

# C++ sources for the main simulator executable (HTTP/WS server)
set(CPP_SOURCES
    trading_engine.cpp
    server.cpp
)

add_executable(simulator ${FORTRAN_SOURCES} ${CPP_SOURCES})

target_link_libraries(simulator PRIVATE
    Crow::Crow
    Threads::Threads
    ${LUA_LIBRARIES}  # if Lua functions are needed in this binary
)

target_include_directories(simulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${asio_SOURCE_DIR}/asio/include
    ${LUA_INCLUDE_DIR}
)

# Create a separate executable for Lua backtesting.
set(LUA_CPP_SOURCES
    trading_engine.cpp   # reuse the trading engine wrappers
    lua_integration.cpp  # main() for Lua integration
)

add_executable(simulator_lua ${FORTRAN_SOURCES} ${LUA_CPP_SOURCES})

target_link_libraries(simulator_lua PRIVATE
    Threads::Threads
    ${LUA_LIBRARIES}
)

target_include_directories(simulator_lua PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${asio_SOURCE_DIR}/asio/include
    ${LUA_INCLUDE_DIR}
)

# Create a separate executable for FIX integration.
set(FIX_CPP_SOURCES
    trading_engine.cpp  # reuse wrappers
    fix_integration.cpp
)

add_executable(simulator_fix ${FORTRAN_SOURCES} ${FIX_CPP_SOURCES})

target_include_directories(simulator_fix PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${asio_SOURCE_DIR}/asio/include
    ${LUA_INCLUDE_DIR}
    /usr/local/include   # Add QuickFIX include directory (adjust if different)
)

target_link_libraries(simulator_fix PRIVATE
    Threads::Threads
    ${LUA_LIBRARIES}
    /usr/local/lib/libquickfix.dylib   # Link against QuickFIX (adjust path/name if needed)
)

