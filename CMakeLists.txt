cmake_minimum_required(VERSION 3.0.0)
project(reaper_interface)

if (MSVC)
    add_compile_options(/W3 /wd5208 /wd4996)
endif()

#set up reaper-sdk and WDL
if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/vendor/reaper-sdk/WDL)
    find_package(Git QUIET)
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    file(CREATE_LINK 
        ${CMAKE_CURRENT_LIST_DIR}/vendor/WDL/WDL
        ${CMAKE_CURRENT_LIST_DIR}/vendor/reaper-sdk/WDL 
        SYMBOLIC)
endif()

#reaper_plugin
add_library(reaper_interface INTERFACE)

include_directories(reaper_interface INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/vendor/reaper-sdk/sdk)

include_directories(reaper_interface INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include)


#reablink sources
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/src)

if (MSVC)
    set_target_properties(reaper_plugin PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})
else()
    set_target_properties(reaper_plugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})
endif()

if (NOT CMAKE_BUILD_TYPE MATCHES "Release")
    if(MSVC)
        set_target_properties(reaper_plugin PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG $ENV{APPDATA}/REAPER/UserPlugins)
        set_target_properties(reaper_plugin PROPERTIES RUNTIME_OUTPUT_DIRECTORY $ENV{APPDATA}/REAPER/UserPlugins)
    elseif(APPLE)
        set_target_properties(reaper_plugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY ~/Library/Application\ Support/REAPER/UserPlugins)
    elseif(UNIX)        
        set_target_properties(reaper_plugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY ~/.config/REAPER/UserPlugins)
    endif()
endif()