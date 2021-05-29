cmake_minimum_required(VERSION 3.0.0)
project(reablink VERSION 0.1.0)
add_library(reablink 
    ReaBlink
    BlinkEngine
    )

include(${CMAKE_SOURCE_DIR}/vendor/link/AbletonLinkConfig.cmake)
target_link_libraries(reablink PRIVATE Ableton::Link)

set_target_properties(reablink PROPERTIES 
    CXX_STANDARD 17 
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    )