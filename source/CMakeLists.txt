add_library(
    reaper_plugin MODULE
    main.cpp
)

add_subdirectory(reablink)

target_link_libraries(
    reaper_plugin PRIVATE
    reablink
)

# if (NOT CMAKE_SYSTEM_NAME MATCHES Windows)
#     add_library(
#         swell OBJECT
#         ${CMAKE_SOURCE_DIR}/vendor/reaper-sdk/WDL/swell/swell-modstub$<IF:$<BOOL:${APPLE}>,.mm,-generic.cpp>
#     )
#     if (CMAKE_SYSTEM_NAME MATCHES Darwin)
#         find_library(APPKIT_LIB AppKit)
#         target_link_libraries(swell PRIVATE ${APPKIT_LIB})
#     endif()
#     target_compile_definitions(swell PUBLIC  SWELL_PROVIDED_BY_APP)
#     target_compile_options(swell PRIVATE -w)
#     target_link_libraries(reaper_plugin PRIVATE swell)
# endif()