target_sources(
  ${PROJECT_NAME}
  PRIVATE
  main.cpp
  AudioEngine.cpp
)

if (WIN32)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("SetThreadDescription")
    set(LINK_WINDOWS_SETTHREADDESCRIPTION 1)
  endif()
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/ReaBlinkConfig.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/ReaBlinkConfig.h
)
