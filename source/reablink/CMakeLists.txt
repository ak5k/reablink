add_library(
        reablink OBJECT
        ReaBlink.cpp
        BlinkEngine.cpp
)

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/ReaBlinkConfig.h.in
    ${PROJECT_BINARY_DIR}/ReaBlinkConfig.h
)

target_include_directories(
    reablink PUBLIC
    ${PROJECT_BINARY_DIR}
)

set_target_properties(
    reablink PROPERTIES
    CXX_STANDARD 17
)

include(${CMAKE_SOURCE_DIR}/vendor/link/AbletonLinkConfig.cmake)
target_link_libraries(
    reablink PRIVATE
    Ableton::Link
)

if(CMAKE_SYSTEM_NAME MATCHES Windows)
    target_compile_definitions(
        reablink PUBLIC
        -DWIN32_LEAN_AND_MEAN
        )
    target_compile_options(
        reablink PRIVATE
        /wd4996
    )
endif()
