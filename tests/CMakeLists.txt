set(NAME unittests-opentxs-cli)

include_directories(
  ${opentxs-cli_SOURCE_DIR}/deps/gtest/include
  ${opentxs-cli_SOURCE_DIR}/src
)

if(MSVC)
  link_directories(
    ${PROJECT_SOURCE_DIR}/../opentxs/build/lib
  )
endif(MSVC)

set(SRC
  tests.cpp
)

add_executable(${NAME} ${SRC})

target_link_libraries(${NAME}
  ${GTEST_BOTH_LIBRARIES}
  otx-base
)

set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${opentxs-cli_BINARY_DIR}/tests)

add_test(${NAME} ${opentxs-cli_BINARY_DIR}/tests/${NAME} --gtest_output=xml:gtestresults.xml)
