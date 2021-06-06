cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(
    ReaBlink
    VERSION 0.3.4
    LANGUAGES CXX
)

# set up reaper-sdk and WDL
if(NOT EXISTS ${PROJECT_SOURCE_DIR}/vendor/reaper-sdk/WDL)
    file(REMOVE 
        ${PROJECT_SOURCE_DIR}/vendor/reaper-sdk/WDL
    )
    find_package(Git QUIET)
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    file(CREATE_LINK 
        ${PROJECT_SOURCE_DIR}/vendor/WDL/WDL
        ${PROJECT_SOURCE_DIR}/vendor/reaper-sdk/WDL 
        SYMBOLIC)
endif()

if (CMAKE_SYSTEM_NAME MATCHES Windows)
    add_compile_options(/W3 /wd5208)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
    set(CMAKE_SHARED_MODULE_SUFFIX ".dylib")
endif()

if (UNIX)
    add_compile_options(-Wall -Werror -Wpedantic -Wextra)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

include_directories(
    SYSTEM
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/vendor/reaper-sdk/sdk
    ${PROJECT_SOURCE_DIR}/vendor/reaper-sdk/WDL
)

# reablink sources
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/source)

# building and packaging etc
if (NOT CMAKE_BUILD_TYPE MATCHES Release)
    if(CMAKE_SYSTEM_NAME MATCHES Windows)
        set_target_properties(reaper_plugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG $ENV{APPDATA}/REAPER/UserPlugins)
        set_target_properties(reaper_plugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE $ENV{APPDATA}/REAPER/UserPlugins)
        set_target_properties(reaper_plugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY $ENV{APPDATA}/REAPER/UserPlugins)
    elseif(CMAKE_SYSTEM_NAME MATCHES Darwin)
        set_target_properties(reaper_plugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY ~/Library/Application\ Support/REAPER/UserPlugins)
    elseif(UNIX)        
        set_target_properties(reaper_plugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY ~/.config/REAPER/UserPlugins)
    endif()
endif()

include(CTest)
enable_testing()

if(CMAKE_OSX_ARCHITECTURES)
    list(JOIN CMAKE_OSX_ARCHITECTURES "-" ARCH_NAME)
elseif(MSVC_CXX_ARCHITECTURE_ID)
    set(ARCH_NAME ${MSVC_CXX_ARCHITECTURE_ID})
else()
    set(ARCH_NAME ${CMAKE_SYSTEM_PROCESSOR})
endif()

if(LINUX_CROSS)
    set(ARCH_NAME ${LINUX_CROSS})
endif()

STRING(TOLOWER "${ARCH_NAME}" ARCH_NAME)

set_target_properties(reaper_plugin PROPERTIES PREFIX "")
set_target_properties(reaper_plugin PROPERTIES OUTPUT_NAME "reaper_reablink-${ARCH_NAME}")

install(
    TARGETS reaper_plugin
    CONFIGURATIONS Release
    LIBRARY 
    DESTINATION "."
    COMPONENT Plugin
)

install(
    DIRECTORY "scripts"
    CONFIGURATIONS Release
    DESTINATION "."
    COMPONENT Scripts
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_NAME ReaBlink)
set(CPACK_PACKAGE_VENDOR ak5k)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Ableton Link REAPER Plug-In Extension")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_OUTPUT_FILE_PREFIX package)

set(CPACK_GENERATOR ZIP)

if (CMAKE_SYSTEM_NAME MATCHES Darwin)
    set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-macosx64)
endif()

if (UNIX AND NOT APPLE)
    STRING(TOLOWER "${CMAKE_SYSTEM_NAME}" CMAKE_SYSTEM_NAME)
    set(CPACK_GENERATOR TGZ)
    set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${ARCH_NAME})
endif()

include(CPack)
