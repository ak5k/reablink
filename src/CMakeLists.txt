cmake_minimum_required(VERSION 3.0.0)
project(reaper_plugin)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_library(reaper_plugin SHARED 
    main)

target_link_libraries(reaper_plugin reaper_interface)

add_subdirectory(reablink)
target_link_libraries(reaper_plugin reablink)

set_target_properties(reaper_plugin PROPERTIES PREFIX "reaper_")
set_target_properties(reaper_plugin PROPERTIES OUTPUT_NAME "reablink")

# if (NOT MSVC)
#     add_library(swell ${CMAKE_SOURCE_DIR}/vendor/reaper-sdk/WDL/swell/swell-modstub$<IF:$<BOOL:${APPLE}>,.mm,-generic.cpp>)
#     target_compile_definitions(swell PUBLIC SWELL_PROVIDED_BY_APP)
#     target_link_libraries(reaper_plugin swell)
# endif()