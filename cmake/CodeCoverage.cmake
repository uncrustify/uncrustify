#
# Code Coverage
#

if ( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
    message( WARNING "Code coverage results with an optimised (non-Debug) build may be misleading" )
endif ( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )

if ( NOT DEFINED CODECOV_OUTPUTFILE )
    set( CODECOV_OUTPUTFILE cmake_coverage.output )
endif ( NOT DEFINED CODECOV_OUTPUTFILE )

if ( NOT DEFINED CODECOV_HTMLOUTPUTDIR )
    set( CODECOV_HTMLOUTPUTDIR coverage_results )
endif ( NOT DEFINED CODECOV_HTMLOUTPUTDIR )

if ( CMAKE_COMPILER_IS_GNUCXX )
    find_program( CODECOV_LCOV lcov )
    find_program( CODECOV_GENHTML genhtml )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -W -Wshadow \
    -Wunused-variable -Wunused-parameter -Wunused-function -Wunused \
    -Wno-system-headers -Wno-deprecated -Woverloaded-virtual -Wwrite-strings \
    -fprofile-arcs -ftest-coverage" )
    link_libraries( gcov )
    set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage" )
    add_custom_target( coverage_init ALL ${CODECOV_LCOV} --base-directory ${PROJECT_SOURCE_DIR}/src
        --directory ${CMAKE_BINARY_DIR} --output-file ${CODECOV_OUTPUTFILE} --no-external --capture --initial
        DEPENDS ${CODECOVERAGE_DEPENDS})
    add_custom_target( coverage          ${CODECOV_LCOV} --base-directory ${PROJECT_SOURCE_DIR}/src
        --directory ${CMAKE_BINARY_DIR} --output-file ${CODECOV_OUTPUTFILE} --no-external --capture)
    add_custom_target( coverage_html     ${CODECOV_GENHTML} -o ${CODECOV_HTMLOUTPUTDIR} ${CODECOV_OUTPUTFILE}
        DEPENDS coverage )
endif ( CMAKE_COMPILER_IS_GNUCXX )
