add_library(
        reablink OBJECT
        main.cpp
        ReaBlink.cpp
        BlinkEngine.cpp
)

target_include_directories(reablink PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(reablink common)

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/ReaBlinkConfig.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/ReaBlinkConfig.h
)

include(${CMAKE_SOURCE_DIR}/vendor/link/AbletonLinkConfig.cmake)

target_link_libraries(
    reablink Ableton::Link
)
