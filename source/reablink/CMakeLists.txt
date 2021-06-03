add_library(
        reablink OBJECT
        ReaBlink.cpp
        BlinkEngine.cpp
)

target_include_directories(
    reablink PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

include(${CMAKE_SOURCE_DIR}/vendor/link/AbletonLinkConfig.cmake)
target_link_libraries(
    reablink PRIVATE
    Ableton::Link
)