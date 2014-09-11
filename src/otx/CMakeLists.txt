# Copyright (c) Cameron Garnham, Monetas AG, 2014

set(name otx)

set(cxx-sources
  main.cpp
)

set(MODULE_NAME otx)
if (WIN32)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/winexe.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/module.rc
    @ONLY
  )

  add_executable(
    ${MODULE_NAME}
    ${cxx-sources}
    ${CMAKE_CURRENT_BINARY_DIR}/module.rc
  )
else()
  add_executable(${MODULE_NAME} ${cxx-sources})
endif()

target_link_libraries(${name} otx-base ${client} ${ext} ${basket} ${core})

install(TARGETS otx DESTINATION bin)
