add_library(
        reablink OBJECT
        ReaBlink.cpp
        BlinkEngine.cpp
)

if(UNIX AND NOT APPLE)
    set_property(
        TARGET reablink PROPERTY
        POSITION_INDEPENDENT_CODE ON
    )
    set(BUILD_SHARED_LIBS ON)
endif()

include(${CMAKE_SOURCE_DIR}/vendor/link/AbletonLinkConfig.cmake)

target_link_libraries(
    reablink PRIVATE
    Ableton::Link
)