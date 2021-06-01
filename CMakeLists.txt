cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
project(
    reablink
    VERSION 0.3.0
    LANGUAGES CXX
)

# set up reaper-sdk and WDL
if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/vendor/reaper-sdk/WDL)
    find_package(Git QUIET)
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    file(CREATE_LINK 
        ${CMAKE_CURRENT_LIST_DIR}/vendor/WDL/WDL
        ${CMAKE_CURRENT_LIST_DIR}/vendor/reaper-sdk/WDL 
        SYMBOLIC)
endif()

# reaper interface
add_library(reaper_interface INTERFACE)
target_include_directories(
    reaper_interface SYSTEM INTERFACE
    ${PROJECT_SOURCE_DIR}/vendor/reaper-sdk/sdk
    ${PROJECT_SOURCE_DIR}/include
)
link_libraries(reaper_interface)

if (CMAKE_SYSTEM_NAME MATCHES Windows)
    add_compile_options(/W3 /wd5208 /wd4996)
endif()

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
    set(CMAKE_SHARED_MODULE_SUFFIX ".dylib")
endif()

if (UNIX)
    add_compile_options(-Wall -Werror -Wpedantic -Wextra)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# reablink sources
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/source)

# building and packaging etc
if (CMAKE_BUILD_TYPE MATCHES "Debug")
    if(CMAKE_SYSTEM_NAME MATCHES Windows)
        set_target_properties(reaper_plugin PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG $ENV{APPDATA}/REAPER/UserPlugins)
        set_target_properties(reaper_plugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG $ENV{APPDATA}/REAPER/UserPlugins)
        set_target_properties(reaper_plugin PROPERTIES RUNTIME_OUTPUT_DIRECTORY $ENV{APPDATA}/REAPER/UserPlugins)
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

if (UNIX AND NOT APPLE)
    set(CPACK_GENERATOR TGZ)
    unset(CPACK_PACKAGE_VERSION)
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
    set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${ARCH_NAME})
endif()

if (CMAKE_SYSTEM_NAME MATCHES Darwin)
    unset(CPACK_PACKAGE_VERSION)
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
    set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-macOS-${CMAKE_OSX_ARCHITECTURES})
endif()

include(CPack)