target_sources(
  ${PROJECT_NAME}
  PRIVATE
  main.cpp
  api.cpp
  engine.cpp
  global_vars.cpp
)

if (WIN32)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("SetThreadDescription")
    set(LINK_WINDOWS_SETTHREADDESCRIPTION 1)
  endif()
endif()