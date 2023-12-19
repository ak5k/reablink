cmake_minimum_required(VERSION 3.22)

include(CheckCXXCompilerFlag)
include(FetchContent)

project(reablink VERSION 0.4.7 LANGUAGES C CXX)
add_library(${PROJECT_NAME} SHARED)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)

# path for external 3rd party library dependencies 
set(PROJECT_LIB_DIR ${PROJECT_SOURCE_DIR}/lib)

FetchContent_Declare(link SYSTEM 
    GIT_REPOSITORY https://github.com/Ableton/link
    GIT_TAG        "Link-3.1.0" # or specify a tag or branch here
    SOURCE_DIR     "${PROJECT_LIB_DIR}/link"
    )

FetchContent_Declare(WDL SYSTEM 
    GIT_REPOSITORY https://github.com/justinfrankel/WDL
    GIT_TAG        "origin/main" # or specify a tag or branch here
    SOURCE_DIR     "${PROJECT_LIB_DIR}/WDL"
    )

FetchContent_Declare(reaper-sdk SYSTEM
    GIT_REPOSITORY https://github.com/justinfrankel/reaper-sdk
    GIT_TAG        "origin/main" # or specify a tag or branch here
    SOURCE_DIR     "${PROJECT_LIB_DIR}/reaper-sdk"
    )

FetchContent_Populate(link)
FetchContent_MakeAvailable(WDL reaper-sdk)

include(${PROJECT_LIB_DIR}/link/AbletonLinkConfig.cmake)
target_link_libraries(${PROJECT_NAME} Ableton::Link)

FILE(GLOB header_paths 
    ${PROJECT_LIB_DIR}/*/include
    ${PROJECT_LIB_DIR}/*/sdk
    )

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${header_paths})

# symlink WDL to reaper-sdk
execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            # source
            "${PROJECT_LIB_DIR}/WDL/WDL"
            # target
            "${PROJECT_LIB_DIR}/reaper-sdk/WDL"
)

if(WIN32)
  target_compile_options(
    ${PROJECT_NAME} 
      PRIVATE 
      /W3 /WX)
else()
  target_compile_options(
    ${PROJECT_NAME}
      PRIVATE
      -Wall -Wextra -Werror -Wpedantic
  )
endif()

include_directories(${PROJECT_NAME} ${header_paths})

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "")
endif()

if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
  set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(WDL REQUIRED)

if(NOT WIN32)
  find_package(SWELL REQUIRED)
endif()

target_link_libraries(${PROJECT_NAME} WDL::WDL)

if(SWELL_FOUND)
  target_link_libraries(${PROJECT_NAME} SWELL::swell)

  # Transpile the Win32 resource file
  find_package(PHP REQUIRED)
  add_custom_command(
    OUTPUT  resource.rc_mac_dlg resource.rc_mac_menu
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_SOURCE_DIR}/resource.rc ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${PHP_EXECUTABLE} ${SWELL_RESGEN} --quiet resource.rc
    DEPENDS ${SWELL_RESGEN}
    MAIN_DEPENDENCY resource.rc
  )
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if(WIN32)
    set(USER_CONFIG_DIR "$ENV{APPDATA}")
  elseif(APPLE)
    set(USER_CONFIG_DIR "~/Library/Application Support")
  else()
    set(USER_CONFIG_DIR "~/.config")
  endif()

  set(CMAKE_INSTALL_PREFIX "${USER_CONFIG_DIR}/REAPER" CACHE PATH
    "REAPER resource path where to install ReaPack" FORCE)
endif()

if(CMAKE_OSX_ARCHITECTURES)
  list(JOIN CMAKE_OSX_ARCHITECTURES "-" ARCH_NAME)
elseif(MSVC_CXX_ARCHITECTURE_ID)
  set(ARCH_NAME ${MSVC_CXX_ARCHITECTURE_ID})
else()
  set(ARCH_NAME ${CMAKE_SYSTEM_PROCESSOR})
endif()

STRING(TOLOWER "${ARCH_NAME}" ARCH_NAME)

# set(CMAKE_CXX_EXTENSIONS OFF)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_VISIBILITY_PRESET "hidden")
# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL     ON)
# set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE        ON)
# set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)

if(VCPKG_TOOLCHAIN)
  set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL Release)
  set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
endif()

# if(WIN32)
  # foreach(arg
  #   CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG
  #   CMAKE_C_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELEASE
  #   CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RELWITHDEBINFO
  #   CMAKE_C_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_MINSIZEREL
  # )
  #   # Embed debug information in the object files
  #   string(REGEX REPLACE "/Z[iI]" "/Z7" ${arg} "${${arg}}")
  # endforeach()

#   add_compile_options(
#     # Exception handling model
#     /EHsc

#     # Enhance optimized debugging
#     $<$<NOT:$<CONFIG:Debug>>:/Zo>

#     # Eliminate duplicate strings
#     $<$<NOT:$<CONFIG:Debug>>:/GF>

#     # Enable function-level linking
#     $<$<NOT:$<CONFIG:Debug>>:/Gy>

#     # Remove unreferenced COMDAT
#     $<$<NOT:$<CONFIG:Debug>>:/Zc:inline>
#   )

#   add_link_options(
#     # Remove unreferenced code
#     $<$<NOT:$<CONFIG:Debug>>:/OPT:REF>

#     # Remove duplicate sections
#     $<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>

#     # Use relative path to the PDB file to avoid exposing the full path
#     /PDBALTPATH:%_PDB%
#   )
# else()
#   add_compile_options(-fsigned-char -fstack-protector-strong -fdiagnostics-color)
# endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
  PREFIX "" # disable the "lib" prefix
  OUTPUT_NAME "reaper_${PROJECT_NAME}-${ARCH_NAME}"
)

add_subdirectory(src)

set(REAPER_USER_PLUGINS "UserPlugins")
install(TARGETS ${PROJECT_NAME}
  COMPONENT ${PROJECT_NAME}
  RUNTIME DESTINATION "${REAPER_USER_PLUGINS}" # Windows .dll
  LIBRARY DESTINATION "${REAPER_USER_PLUGINS}" # Linux .so/macOS .dylib
)
