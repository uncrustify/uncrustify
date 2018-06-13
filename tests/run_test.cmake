#
# This is a wrapper CMake script to run one uncrustify test because CMake
# add_test can't launch multiple processes on its own, and so doesn't directly
# allow a complex test scenario.
#

cmake_minimum_required(VERSION 2.8)

function(require)
  foreach(var IN LISTS ARGN)
    if(NOT ${var})
      message(FATAL_ERROR "Variable ${var} not defined")
    endif()
  endforeach()
endfunction()

function(run_test name config input output result)
  execute_process(
    COMMAND ${TEST_PROGRAM}
      -l ${TEST_LANG}
      -c ${config}
      -f ${input}
      -o ${result}
    WORKING_DIRECTORY ${TEST_DIR}
    RESULT_VARIABLE uncrustify_error_code
    OUTPUT_QUIET
  )

  if(uncrustify_error_code)
    message(SEND_ERROR
      "Uncrustify error_code ${uncrustify_error_code} (${name})"
    )
  else()
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E compare_files ${result} ${output}
      RESULT_VARIABLE files_are_different
    )
    if(files_are_different)
      message(WARNING
        "MISMATCH (${name}): ${result} does not match ${output}"
      )
      if(GIT_EXECUTABLE)
        execute_process(
          COMMAND ${GIT_EXECUTABLE}
            diff --no-index
            ${output}
            ${result}
        )
      else()
        execute_process(
          COMMAND ${CMAKE_COMMAND} -E echo "the result file:"
        )
        execute_process(
          COMMAND ${CMAKE_COMMAND} -E copy ${result} /dev/stderr
        )
      endif()
      message(SEND_ERROR "Uncrustify MISMATCH (${name})")
    endif()
  endif()
endfunction()

require(
  TEST_PROGRAM
  TEST_LANG
  TEST_CONFIG
  TEST_RERUN_CONFIG
  TEST_INPUT
  TEST_OUTPUT
  TEST_RESULT
  TEST_RESULT_2
  TEST_DIR
  TEST_RERUN_OUTPUT
)

# Run first pass
run_test(
  "Pass 1"
  ${TEST_CONFIG}
  ${TEST_DIR}/${TEST_INPUT}
  ${TEST_DIR}/${TEST_OUTPUT}
  ${TEST_DIR}/${TEST_RESULT}
)

# Re-run with the output file as the input to check stability
run_test(
  "Pass 2"
  ${TEST_RERUN_CONFIG}
  ${TEST_DIR}/${TEST_RERUN_OUTPUT}
  ${TEST_DIR}/${TEST_RERUN_OUTPUT}
  ${TEST_DIR}/${TEST_RESULT_2}
)
