target_sources(
  ${PROJECT_NAME}
  PRIVATE
  main.cpp
  ReaBlink.cpp
  BlinkEngine.cpp
)

if (WIN32)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("SetThreadDescription")
    set(LINK_WINDOWS_SETTHREADDESCRIPTION 1)
  endif()
endif()


# target_link_libraries(reablink PRIVATE reaper_plugin)

# target_include_directories(reablink PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# if(WIN32)
#   target_compile_options(reablink PRIVATE
#     /W3 /WX /wd4996 # /wd4706 /wd4100 /wd4996 /wd4505
#     )
# else()
#   target_compile_options(reablink PRIVATE
#     -Wall -Wextra -Werror -Wpedantic)
# endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/ReaBlinkConfig.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/ReaBlinkConfig.h
)
