cmake_minimum_required(VERSION 2.8)

find_package(Git QUIET)

function(add_uncrustify_test
  name group lang input config rerun_config expected rerun_expected
)
  add_test(NAME ${name}
    COMMAND ${PYTHON_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/run_test.py
      ${name}
      --executable     $<TARGET_FILE:uncrustify>
      --lang           ${lang}
      --input          ${input}
      --config         ${config}
      --expected       ${expected}
      --rerun-config   ${rerun_config}
      --rerun-expected ${rerun_expected}
      -d --git         ${GIT_EXECUTABLE}
  )
  set_tests_properties(${name}
    PROPERTIES LABELS "${group}"
  )
endfunction()

foreach(test_group c-sharp c cpp d java pawn objective-c vala ecma imported)
  # Create dependency on test file so CMake re-runs if changed
  configure_file("${test_group}.test" "${test_group}.test" COPYONLY)
  file(READ "${test_group}.test" content)
  string(REPLACE "\n" ";" lines "${content}")
  foreach(line ${lines})
    string(STRIP "${line}" line)
    string(SUBSTRING "${line}" 0 1 first)
    # trailing space avoids double evaluation with older CMake versions
    if(NOT "${first} " STREQUAL "# ")
      if("${line}" MATCHES "^([0-9]+)([!]?)[ ]+([^ ]+)[ ]+([^ ]+)([ ]+([^ ]+))?$")
        set(test_number "${CMAKE_MATCH_1}")
        set(test_name "${test_group}:${test_number}")

        set(test_config ${CMAKE_CURRENT_SOURCE_DIR}/config/${CMAKE_MATCH_3})
        set(test_input ${CMAKE_CURRENT_SOURCE_DIR}/input/${CMAKE_MATCH_4})

        get_filename_component(test_dir ${CMAKE_MATCH_4} PATH)
        get_filename_component(test_file ${CMAKE_MATCH_4} NAME)

        set(test_expected ${test_dir}/${CMAKE_MATCH_1}-${test_file})
        set(test_expected ${CMAKE_CURRENT_SOURCE_DIR}/expected/${test_expected})

        if("${CMAKE_MATCH_2} " STREQUAL "! ")
          string(REGEX REPLACE
            "\\.[^.]*" ".rerun.cfg"
            test_rerun_config ${test_config}
          )
          string(REGEX REPLACE
            "\\.([^.]*)" ".rerun.\\1"
            test_rerun_expected ${test_expected}
          )
         message(STATUS "input: ${test_input}")
         message(STATUS "expected: ${test_expected}")
         message(STATUS "rerun_expected: ${test_rerun_expected}")
        else()
          set(test_rerun_config ${test_config})
          set(test_rerun_expected ${test_expected})
        endif()

        if(NOT "${CMAKE_MATCH_6} " STREQUAL " ")
          set(test_lang "${CMAKE_MATCH_6}")
        else()
          string(TOUPPER "${test_dir}" test_lang)
        endif()

        add_uncrustify_test(
          ${test_name} ${test_group} ${test_lang} ${test_input}
          ${test_config} ${test_rerun_config}
          ${test_expected} ${test_rerun_expected}
        )
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

add_test(NAME sanity COMMAND uncrustify --help)
