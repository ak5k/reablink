add_library(
    reaper_plugin SHARED 
    main.cpp
)

# set_target_properties(reaper_plugin PROPERTIES 
#     CXX_STANDARD 17 
#     CXX_STANDARD_REQUIRED ON
#     CXX_EXTENSIONS OFF
#     )

target_link_libraries(
    reaper_plugin
    reaper_interface
)

add_subdirectory(reablink)
target_link_libraries(
    reaper_plugin
    reablink
)


if(CMAKE_OSX_ARCHITECTURES)
    list(JOIN CMAKE_OSX_ARCHITECTURES "-" ARCH_NAME)
elseif(MSVC_CXX_ARCHITECTURE_ID)
    set(ARCH_NAME ${MSVC_CXX_ARCHITECTURE_ID})
else()
    set(ARCH_NAME ${CMAKE_SYSTEM_PROCESSOR})
endif()

STRING(TOLOWER "${ARCH_NAME}" ARCH_NAME)

set_target_properties(reaper_plugin PROPERTIES PREFIX "")
set_target_properties(reaper_plugin PROPERTIES OUTPUT_NAME "reaper_reablink-${ARCH_NAME}")

# if (NOT MSVC)
#     add_library(swell ${CMAKE_SOURCE_DIR}/vendor/reaper-sdk/WDL/swell/swell-modstub$<IF:$<BOOL:${APPLE}>,.mm,-generic.cpp>)
#     target_compile_definitions(swell PUBLIC SWELL_PROVIDED_BY_APP)
#     target_link_libraries(reaper_plugin swell)
# endif()