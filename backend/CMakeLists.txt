cmake_minimum_required(VERSION 3.10)
project(flashTrading LANGUAGES Fortran CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(Lua REQUIRED)

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

# Fortran source (advanced order book)
set(FORTRAN_SOURCES
    advanced_order_book.f90
)

# C++ sources for the main simulator executable (HTTP/WS server)
set(SIMULATOR_CPP_SOURCES
    trading_engine.cpp
    server.cpp
)

add_executable(simulator ${FORTRAN_SOURCES} ${SIMULATOR_CPP_SOURCES})
target_link_libraries(simulator PRIVATE
    Crow::Crow
    Threads::Threads
    ${LUA_LIBRARIES}
)
target_include_directories(simulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${asio_SOURCE_DIR}/asio/include
    ${LUA_INCLUDE_DIR}
)

# Lua backtesting executable
set(LUA_CPP_SOURCES
    trading_engine.cpp
    lua_integration.cpp
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

# FIX integration executable (using QuickFIX from Homebrew on Apple M2)
set(FIX_CPP_SOURCES
    trading_engine.cpp
    fix_integration.cpp
)
add_executable(simulator_fix ${FORTRAN_SOURCES} ${FIX_CPP_SOURCES})
target_include_directories(simulator_fix PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${asio_SOURCE_DIR}/asio/include
    ${LUA_INCLUDE_DIR}
    /usr/local/include    # Assuming QuickFIX headers are here
)

target_link_libraries(simulator_fix PRIVATE
    Threads::Threads
    ${LUA_LIBRARIES}
    /usr/local/lib/libquickfix.dylib   # Adjust if your QuickFIX library is elsewhere
)

# Optional feed executable (HTTP-based feed)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/feed.cpp")
    add_executable(simulator_feed feed.cpp)
    target_link_libraries(simulator_feed PRIVATE
        "-lcurl"
    )
    target_include_directories(simulator_feed PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()


