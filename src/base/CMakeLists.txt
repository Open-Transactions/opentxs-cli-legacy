# Copyright (c) Cameron Garnham, Monetas AG, 2014

set(name otx-base)

set(cxx-sources
  ccolor.cpp
  cmd.cpp
  cmd_tests.cpp
  cmd_tree.cpp
  daemon_tools.cpp
  example_coding.cpp
  otcli.cpp
  othint.cpp
  runoptions.cpp
  table_printer.cpp
  template.cpp
  text.cpp
  useot.cpp
  utils.cpp
)

file(GLOB cxx-headers "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

set(dependency_include_dir
  ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/
)

include_directories(SYSTEM
  ${dependency_include_dir}
)

set(MODULE_NAME otx-base)
if(WIN32)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/windll.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/module.rc
    @ONLY
  )

  # we have an extra Windows-specific file to compile
  add_library(${MODULE_NAME} STATIC
    ${cxx-sources}
    ${cxx-headers}
    ${CMAKE_CURRENT_BINARY_DIR}/module.rc
  )
else()
  add_library(${MODULE_NAME} STATIC
    ${cxx-sources}
    ${cxx-headers}
  )
endif()

if(WIN32)
  target_link_libraries(${name} ${client} ${ext} ${cash} ${basket} ${core} irrxml)
else()
  target_link_libraries(${name} ${client} ${ext} ${cash} ${basket} ${core})
endif(WIN32)
