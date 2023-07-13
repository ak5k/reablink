add_library(
        reablink OBJECT
        main.cpp
        ReaBlink.cpp
        BlinkEngine.cpp
)

if (WIN32)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LINK_WINDOWS_SETTHREADDESCRIPTION 1)
  endif()
endif()

include(${CMAKE_SOURCE_DIR}/vendor/link/AbletonLinkConfig.cmake)
target_link_libraries(
    reablink PRIVATE Ableton::Link
)

target_link_libraries(reablink PRIVATE reaper_plugin)

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
