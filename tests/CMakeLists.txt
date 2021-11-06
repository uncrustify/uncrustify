cmake_minimum_required(VERSION 2.8.12)

find_package(Git QUIET)

# sorted by lexicographic order
set(test_suites
    c-sharp.test
    c.test
    cpp.test
    d.test
    ecma.test
    imported.test
    java.test
    objective-c.test
    pawn.test
    vala.test
)

if (NOT ENABLE_CODECOVERAGE)
  list(APPEND test_suites staging.test)
endif()

set(stdoc
  "Create a separate CTest test for each test case"
  " this is slower, especially with Python 3"
)
option("UNCRUSTIFY_SEPARATE_TESTS" "${stdoc}" OFF)
if (UNCRUSTIFY_SEPARATE_TESTS)
  set(tests_ctest_file "${CMAKE_CURRENT_BINARY_DIR}/tests.cmake")
  set_property(DIRECTORY PROPERTY TEST_INCLUDE_FILE ${tests_ctest_file})

  add_custom_command(
    OUTPUT ${tests_ctest_file}
    COMMAND ${PYTHON_EXECUTABLE} run_format_tests.py
      --executable $<TARGET_FILE:uncrustify>
      --git ${GIT_EXECUTABLE}
      --python ${PYTHON_EXECUTABLE}
      --write-ctest ${tests_ctest_file}
      --cmake-config "$<CONFIGURATION>"
    VERBATIM
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${test_suites}
  )

  add_custom_target(register_tests ALL DEPENDS ${tests_ctest_file})
else()
  foreach(suite IN LISTS test_suites)
    string(REPLACE ".test" "" lang "${suite}")
    add_test(NAME ${lang}
      COMMAND ${PYTHON_EXECUTABLE} run_format_tests.py ${lang}
        --executable $<TARGET_FILE:uncrustify>
        -d --git ${GIT_EXECUTABLE}
        --result-dir ${CMAKE_CURRENT_BINARY_DIR}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
  endforeach()
endif()

if (NOT ENABLE_CODECOVERAGE)
  add_test(
    NAME sources_format
    COMMAND ${PYTHON_EXECUTABLE} run_sources_tests.py
      --executable $<TARGET_FILE:uncrustify>
      -d --git ${GIT_EXECUTABLE}
      --result-dir ${CMAKE_CURRENT_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endif()

add_test(
  NAME cli_options
  COMMAND ${PYTHON_EXECUTABLE}
    test_cli_options.py
    --config $<CONFIG>
    --build ${uncrustify_BINARY_DIR}
    --diff
    --test ${CMAKE_CURRENT_BINARY_DIR}/cli
  ${_configs}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cli
)

add_custom_target(update-cli-options
  COMMAND ${PYTHON_EXECUTABLE}
    test_cli_options.py
    --config $<CONFIG>
    --build ${uncrustify_BINARY_DIR}
    --apply
    --test ${CMAKE_CURRENT_BINARY_DIR}/cli
  DEPENDS uncrustify
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cli
)

add_test(NAME sanity COMMAND uncrustify --help)
