cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
project(
    reablink
    VERSION 0.3.0
    )

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
target_include_directories(
    reaper_interface SYSTEM INTERFACE
    ${PROJECT_SOURCE_DIR}/vendor/reaper-sdk/sdk
    ${PROJECT_SOURCE_DIR}/include
)
#reablink sources
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/source)

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    if(MSVC)
        set_target_properties(reaper_plugin PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG $ENV{APPDATA}/REAPER/UserPlugins)
        set_target_properties(reaper_plugin PROPERTIES RUNTIME_OUTPUT_DIRECTORY $ENV{APPDATA}/REAPER/UserPlugins)
    elseif(APPLE)
        set_target_properties(reaper_plugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY ~/Library/Application\ Support/REAPER/UserPlugins)
    elseif(UNIX)        
        set_target_properties(reaper_plugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY ~/.config/REAPER/UserPlugins)
    endif()
endif()

include(CTest)
enable_testing()

install(
    TARGETS reaper_plugin
    CONFIGURATIONS Release
    RUNTIME 
    DESTINATION "."
)

install(
    DIRECTORY "scripts"
    CONFIGURATIONS Release
    DESTINATION "."
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_NAME ReaBlink)
set(CPACK_PACKAGE_VENDOR ak5k)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Ableton Link REAPER Plug-In Extension")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_OUTPUT_FILE_PREFIX package)
set(CPACK_GENERATOR ZIP)

include(CPack)