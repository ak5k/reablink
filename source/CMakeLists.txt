add_library(
    reaper_plugin MODULE
    main.cpp
)

add_subdirectory(reablink)

target_link_libraries(
    reaper_plugin PRIVATE
    reablink
)
