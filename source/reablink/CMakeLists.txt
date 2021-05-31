add_library(
    reablink
    ReaBlink.cpp
    BlinkEngine.cpp
)

target_link_libraries(
    reablink PRIVATE
    reaper_interface
)

include(${CMAKE_SOURCE_DIR}/vendor/link/AbletonLinkConfig.cmake)
target_link_libraries(
    reablink PRIVATE
    Ableton::Link
)