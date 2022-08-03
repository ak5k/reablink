add_library(
        reablink OBJECT
        main.cpp
        ReaBlink.cpp
        BlinkEngine.cpp
)

include(${CMAKE_SOURCE_DIR}/vendor/link/AbletonLinkConfig.cmake)
target_compile_definitions(Ableton::Link INTERFACE LINK_WINDOWS_SETTHREADDESCRIPTION)
target_link_libraries(
    reablink Ableton::Link
)

target_link_libraries(reablink reaper_plugin)

target_include_directories(reablink PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if(WIN32)
  target_compile_options(reablink PRIVATE
    /W3 /WX /wd4996 # /wd4706 /wd4100 /wd4996 /wd4505
    )
else()
  target_compile_options(reablink PRIVATE
    -Wall -Wextra -Werror -Wpedantic)
endif()

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/ReaBlinkConfig.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/ReaBlinkConfig.h
)
