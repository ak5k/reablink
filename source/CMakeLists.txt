add_library(
    reaper_plugin MODULE
    main.cpp
)

target_link_libraries(
    reaper_plugin PRIVATE
    reaper_interface
)

add_subdirectory(reablink)
target_link_libraries(
    reaper_plugin PRIVATE
    reablink
)
