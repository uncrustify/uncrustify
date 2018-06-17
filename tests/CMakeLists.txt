cmake_minimum_required(VERSION 2.8)

find_package(Git QUIET)

function(add_uncrustify_test name lang config rerun_config input result result_2 output rerun_output)
  add_test(NAME ${name}
    COMMAND ${CMAKE_COMMAND}
      -DTEST_PROGRAM=$<TARGET_FILE:uncrustify>
      -DTEST_LANG=${lang}
      -DTEST_CONFIG=${config}
      -DTEST_RERUN_CONFIG=${rerun_config}
      -DTEST_INPUT=${input}
      -DTEST_RESULT=${result}
      -DTEST_RESULT_2=${result_2}
      -DTEST_OUTPUT=${output}
      -DTEST_RERUN_OUTPUT=${rerun_output}
      -DTEST_DIR=${PROJECT_SOURCE_DIR}/tests
      -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
      -P ${PROJECT_SOURCE_DIR}/tests/run_test.cmake
  )
  set_tests_properties(${name}
    PROPERTIES LABELS "${lang}"
  )
endfunction()

foreach(test_lang c-sharp c cpp d java pawn objective-c vala ecma imported regression)
  # Create dependency on test file so CMake re-runs if changed
  configure_file("${test_lang}.test" "${test_lang}.test" COPYONLY)
  file(READ "${test_lang}.test" content)
  string(REPLACE "\n" ";" lines "${content}")
  foreach(line ${lines})
    string(STRIP "${line}" line)
    string(SUBSTRING "${line}" 0 1 first)
    # trailing space avoids double evaluation with older CMake versions
    if(NOT "${first} " STREQUAL "# ")
      if("${line}" MATCHES "^([0-9]+)([!]?)[ ]+([^ ]+)[ ]+([^ ]+)$")
        set(test_number "${CMAKE_MATCH_1}")
        set(test_name "${test_lang}_${test_number}")

        set(test_config config/${CMAKE_MATCH_3})
        set(test_input input/${CMAKE_MATCH_4})

        get_filename_component(test_dir ${CMAKE_MATCH_4} PATH)
        get_filename_component(test_file ${CMAKE_MATCH_4} NAME)

        set(test_expected ${test_dir}/${CMAKE_MATCH_1}-${test_file})
        set(test_output output/${test_expected})
        set(test_result results/${test_expected})
        set(test_result_2 results_2/${test_expected})

        if("${CMAKE_MATCH_2} " STREQUAL "! ")
          string(REGEX REPLACE "\\.[^.]*" ".rerun.cfg"  test_rerun_config ${test_config})
          string(REGEX REPLACE "\\.[^.]*" ".rerun.lang" test_rerun_output ${test_output})
        else()
          set(test_rerun_config ${test_config})
          set(test_rerun_output ${test_output})
        endif()

        add_uncrustify_test(${test_name} ${test_dir} ${test_config} ${test_rerun_config}
          ${test_input} ${test_result} ${test_result_2} ${test_output} ${test_rerun_output})

      endif()
    endif()
  endforeach()
endforeach()

if (CMAKE_CONFIGURATION_TYPES)
  # Not really, but just to make the following if() true
  set(_build_type release)
  set(_configs CONFIGURATIONS Release RelWithDebInfo MinSizeRel)
else()
  string(TOLOWER "${CMAKE_BUILD_TYPE}" _build_type)
  set(_configs)
endif()

# Only enable test_cli_options stuff if building a release configuration,
# or using a multi-configuration generator (note: in the latter case, these
# will fail if a release configuration has not been built)
if (_build_type MATCHES "release|relwithdebinfo|minsizerel")
  add_test(
    NAME cli_options
    COMMAND ${PYTHON_EXECUTABLE}
      test_cli_options.py
      --build ${uncrustify_BINARY_DIR}
      --cache ${CMAKE_BINARY_DIR}/CMakeCache.txt
      --diff
    ${_configs}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cli
  )

  add_custom_target(update_cli_options
    COMMAND ${PYTHON_EXECUTABLE}
      test_cli_options.py
      --build ${uncrustify_BINARY_DIR}
      --cache ${CMAKE_BINARY_DIR}/CMakeCache.txt
      --apply
    DEPENDS uncrustify
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cli
  )
endif()
