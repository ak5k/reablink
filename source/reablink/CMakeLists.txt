add_library(
        reablink OBJECT
        ReaBlink.cpp
        BlinkEngine.cpp
)

include(${CMAKE_SOURCE_DIR}/vendor/link/AbletonLinkConfig.cmake)
target_link_libraries(
    reablink PRIVATE
    Ableton::Link
)