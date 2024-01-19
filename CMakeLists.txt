cmake_minimum_required(VERSION 3.5)

project(uncrustify)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
  message(FATAL_ERROR "
    In-source builds are not supported, please remove the `CMakeFiles'
    folder and `CMakeCache.txt', and create a folder for the build:
    mkdir build; cd build; cmake ..
  ")
endif()

include(CheckCXXCompilerFlag)
include(CheckIncludeFileCXX)
include(CheckSymbolExists)
include(CheckCXXSymbolExists)
include(CheckTypeSize)
include(CTest)

if( ${CMAKE_VERSION} VERSION_LESS "3.12" )
    find_package( PythonInterp )
    if( NOT PYTHON_EXECUTABLE )
        message( FATAL_ERROR "Python is required, but was not found on your system" )
    endif()
 else( )
    find_package(Python3 REQUIRED)
    set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
endif()

#
# Check compiler flags
#
if(MSVC)
  add_definitions(/D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_WARNINGS /wd4267)
  add_definitions(/utf-8)
elseif(CMAKE_COMPILER_IS_GNUCXX)
  set(gcc_warning_flags
    -Wall
    -Wextra
    -Wshadow
    -Wpointer-arith
    -Wcast-qual
    -Wcast-align
    -Wc++11-extensions
  )
  foreach(flag ${gcc_warning_flags})
    string(REGEX REPLACE "[^a-zA-Z0-9]+" "_" flag_var "CXXFLAG_${flag}")
    CHECK_CXX_COMPILER_FLAG("${flag}" ${flag_var})
    if(${flag_var})
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
    endif()
    unset(flag_var)
  endforeach()
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")
  endif()
  unset(gcc_warning_flags)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-deprecated-declarations")
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

if(ENABLE_SANITIZER)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -fno-omit-frame-pointer -fsanitize=${ENABLE_SANITIZER}")
endif()

#set(UNCRUSTIFY_SEPARATE_TESTS "True")

include_directories(
  ${PROJECT_BINARY_DIR}/src
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}
)

#
# Determine config
#
if(WIN32)
  # Windows builds use src/windows_compat.h instead of config.h
else()
  # Generate config.h
  set(avail_headers "")

  set(headers
    inttypes.h
    memory.h
    stdint.h
    stdlib.h
    strings.h
    string.h
    sys/stat.h
    sys/types.h
    unistd.h
    utime.h
  )
  foreach(header ${headers})
    string(TOUPPER "${header}" header_uc)
    string(REGEX REPLACE "[^A-Z0-9_]" "_" include_var "HAVE_${header_uc}")
    check_include_file_cxx("${header}" ${include_var})
    if(${include_var})
      list(APPEND avail_headers ${header})
    endif()
    unset(include_var)
    unset(header_uc)
  endforeach()
  unset(headers)

  check_include_file("stdbool.h" HAVE_STDBOOL_H)

  set(symbols
    memset
    strcasecmp
    strchr
    strdup
    strerror
    strtol
    strtoul
  )
  foreach(symbol ${symbols})
    string(TOUPPER "${symbol}" symbol_uc)
    string(REGEX REPLACE "[^A-Z0-9_]" "_" symbol_var "HAVE_${symbol_uc}")
    check_cxx_symbol_exists("${symbol}" "${avail_headers}" ${symbol_var})
    unset(symbol_var)
    unset(symbol_uc)
  endforeach()
  unset(symbols)

  unset(avail_headers)

  check_type_size(_Bool _BOOL LANGUAGE C)

  configure_file(src/config.h.in config.h @ONLY)
endif()

#
# Generate uncrustify_version.h
#

set(UNCRUSTIFY_VERSION "0.78.1_f")

option(NoGitVersionString "Do not use make_version.py and git to build a version string" OFF)
if(NoGitVersionString)
  configure_file(src/uncrustify_version.h.in uncrustify_version.h @ONLY)
  add_custom_target(generate_version_header) # Dummy target
else()
  # Add target to generate version header;
  # do this every build to ensure git SHA is up to date
  add_custom_target(generate_version_header
    BYPRODUCTS "${PROJECT_BINARY_DIR}/uncrustify_version.h"
    COMMAND
      ${CMAKE_COMMAND}
      -D PYTHON_EXECUTABLE:STRING=${PYTHON_EXECUTABLE}
      -D SOURCE_DIR:PATH="${PROJECT_SOURCE_DIR}"
      -D INPUT:PATH="${PROJECT_SOURCE_DIR}/src/uncrustify_version.h.in"
      -D OUTPUT:PATH="${PROJECT_BINARY_DIR}/uncrustify_version.h"
      -D UNCRUSTIFY_VERSION:STRING="${UNCRUSTIFY_VERSION}"
      -P ${PROJECT_SOURCE_DIR}/cmake/GenerateVersionHeader.cmake
    COMMENT "Generating version header"
  )
  set_source_files_properties(
    "${PROJECT_BINARY_DIR}/uncrustify_version.h"
    PROPERTIES GENERATED TRUE
  )
endif()

#
# Generate token_names.h
#
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/token_names.h"
  COMMAND ${CMAKE_COMMAND}
    "-Dsrc_file=${PROJECT_SOURCE_DIR}/src/token_enum.h"
    "-Ddst_file=${CMAKE_CURRENT_BINARY_DIR}/token_names.h"
    -P "${PROJECT_SOURCE_DIR}/cmake/GenerateTokenNames.cmake"
  MAIN_DEPENDENCY src/token_enum.h
  COMMENT "Generating token_names.h"
)

# Set up commands for generated source files
function(py_gen OUTPUT SCRIPT INPUT)
  set(out "${PROJECT_BINARY_DIR}/src/${OUTPUT}")
  set(deps "${PROJECT_SOURCE_DIR}/src/${INPUT}")
  get_filename_component(outdir "${out}" DIRECTORY)
  foreach(arg IN LISTS ARGN)
    if (IS_ABSOLUTE "${arg}")
      list(APPEND deps "${arg}")
    else()
      list(APPEND deps "${PROJECT_SOURCE_DIR}/src/${arg}")
    endif()
  endforeach()

  add_custom_command(
    OUTPUT "${out}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${outdir}"
    COMMAND ${PYTHON_EXECUTABLE}
      "${PROJECT_SOURCE_DIR}/scripts/${SCRIPT}"
      "${out}"
      ${deps}
    DEPENDS ${deps} "${PROJECT_SOURCE_DIR}/scripts/${SCRIPT}"
    MAIN_DEPENDENCY src/${INPUT}
    COMMENT "Generating ${OUTPUT}"
  )
endfunction()

py_gen(punctuator_table.h
  make_punctuator_table.py
  symbols_table.h
)

py_gen(options.cpp
  make_options.py
  options.h
  options.cpp.in
)

py_gen(option_enum.h
  make_option_enum.py
  option.h
  option_enum.h.in
)

py_gen(option_enum.cpp
  make_option_enum.py
  option.h
  option_enum.cpp.in
)

py_gen(../etc/uncrustify.xml
  make_katehl.py
  ../etc/uncrustify.xml.in
  "${PROJECT_BINARY_DIR}/uncrustify_version.h"
  options.h
  option.h
  token_enum.h
)

#
# Uncrustify
#
set(uncrustify_sources
  src/align_add.cpp
  src/align_asm_colon.cpp
  src/align_assign.cpp
  src/align_braced_init_list.cpp
  src/align.cpp
  src/align_eigen_comma_init.cpp
  src/align_func_params.cpp
  src/align_func_proto.cpp
  src/align_init_brace.cpp
  src/align_left_shift.cpp
  src/align_log_al.cpp
  src/align_nl_cont.cpp
  src/align_oc_decl_colon.cpp
  src/align_oc_msg_colons.cpp
  src/align_oc_msg_spec.cpp
  src/align_preprocessor.cpp
  src/align_same_func_call_params.cpp
  src/align_stack.cpp
  src/align_struct_initializers.cpp
  src/align_tab_column.cpp
  src/align_tools.cpp
  src/align_trailing_comments.cpp
  src/align_typedefs.cpp
  src/align_var_def_brace.cpp
  src/annotations_newlines.cpp
  src/args.cpp
  src/backup.cpp
  src/blank_line.cpp
  src/brace_cleanup.cpp
  src/braces.cpp
  src/calculate_closing_brace_position.cpp
  src/can_increase_nl.cpp
  src/change_int_types.cpp
  src/check_double_brace_init.cpp
  src/check_template.cpp
  src/chunk.cpp
  src/ChunkStack.cpp
  src/collapse_empty_body.cpp
  src/combine.cpp
  src/combine_fix_mark.cpp
  src/combine_labels.cpp
  src/combine_skip.cpp
  src/combine_tools.cpp
  src/compat_posix.cpp
  src/compat_win32.cpp
  src/cs_top_is_question.cpp
  src/detect.cpp
  src/do_it_newlines_func_pre_blank_lines.cpp
  src/double_newline.cpp
  src/enum_cleanup.cpp
  src/EnumStructUnionParser.cpp
  src/flag_braced_init_list.cpp
  src/flag_decltype.cpp
  src/flag_parens.cpp
  src/get_closing_brace.cpp
  src/ifdef_over_whole_file.cpp
  src/indent.cpp
  src/is_class_one_liner.cpp
  src/is_func_call_or_def.cpp
  src/is_func_proto_group.cpp
  src/is_var_def.cpp
  src/keywords.cpp
  src/lang_pawn.cpp
  src/language_names.cpp
  src/language_tools.cpp
  src/logger.cpp
  src/logmask.cpp
  src/log_rules.cpp
  src/mark_change.cpp
  src/mark_functor.cpp
  src/mark_question_colon.cpp
  src/md5.cpp
  src/newline_add.cpp
  src/newline_after.cpp
  src/newline_before_return.cpp
  src/newline_case.cpp
  src/newline_del_between.cpp
  src/newline_end_newline.cpp
  src/newline_force.cpp
  src/newline_func.cpp
  src/newline_iarf.cpp
  src/newline_min_after.cpp
  src/newline_oc_msg.cpp
  src/newlines_between.cpp
  src/newlines_brace_pair.cpp
  src/newlines_chunk_pos.cpp
  src/newlines_class_colon_pos.cpp
  src/newlines_cleanup.cpp
  src/newlines_cuddle_uncuddle.cpp
  src/newlines_do_else.cpp
  src/newlines_double_space_struct_enum_union.cpp
  src/newlines_eat_start_end.cpp
  src/newlines_enum.cpp
  src/newlines_func_pre_blank_lines.cpp
  src/newlines_functions_remove_extra_blank_lines.cpp
  src/newlines_if_for_while_switch.cpp
  src/newlines_if_for_while_switch_post_blank_lines.cpp
  src/newlines_if_for_while_switch_pre_blank_lines.cpp
  src/newlines_namespace.cpp
  src/newlines_remove_disallowed.cpp
  src/newlines_remove_newlines.cpp
  src/newlines_sparens.cpp
  src/newlines_squeeze_ifdef.cpp
  src/newlines_squeeze_paren_close.cpp
  src/newlines_struct_union.cpp
  src/newline_template.cpp
  src/newline_template_option.cpp
  src/newline_var_def_blk.cpp
  src/nl_create_list_liner.cpp
  src/nl_create_one_liner.cpp
  src/nl_handle_define.cpp
  src/one_liner_nl_ok.cpp
  src/option.cpp
  src/options_for_QT.cpp
  src/output.cpp
  src/parameter_pack_cleanup.cpp
  src/parens.cpp
  src/parent_for_pp.cpp
  src/parsing_frame.cpp
  src/parsing_frame_stack.cpp
  src/pcf_flags.cpp
  src/pragma_cleanup.cpp
  src/punctuators.cpp
  src/quick_align_again.cpp
  src/reindent_line.cpp
  src/remove_duplicate_include.cpp
  src/remove_extra_returns.cpp
  src/remove_next_newlines.cpp
  src/rewrite_infinite_loops.cpp
  src/semicolons.cpp
  src/setup_newline_add.cpp
  src/sorting.cpp
  src/space.cpp
  src/token_is_within_trailing_return.cpp
  src/tokenize_cleanup.cpp
  src/tokenize.cpp
  src/too_big_for_nl_max.cpp
  src/unc_ctype.cpp
  src/uncrustify.cpp
  src/uncrustify_emscripten.cpp
  src/uncrustify_types.cpp
  src/unc_text.cpp
  src/unc_tools.cpp
  src/undo_one_liner.cpp
  src/unicode.cpp
  src/universalindentgui.cpp
  src/width.cpp
  ${PROJECT_BINARY_DIR}/src/options.cpp
  ${PROJECT_BINARY_DIR}/src/option_enum.cpp
)

set(uncrustify_headers
  src/add_space_table.h
  src/align_add.h
  src/align_asm_colon.h
  src/align_assign.h
  src/align_braced_init_list.h
  src/align.h
  src/align_eigen_comma_init.h
  src/align_func_params.h
  src/align_func_proto.h
  src/align_init_brace.h
  src/align_left_shift.h
  src/align_log_al.h
  src/align_nl_cont.h
  src/align_oc_decl_colon.h
  src/align_oc_msg_colons.h
  src/align_oc_msg_spec.h
  src/align_preprocessor.h
  src/align_same_func_call_params.h
  src/align_stack.h
  src/align_struct_initializers.h
  src/align_tab_column.h
  src/align_tools.h
  src/align_trailing_comments.h
  src/align_typedefs.h
  src/align_var_def_brace.h
  src/annotations_newlines.h
  src/args.h
  src/backup.h
  src/base_types.h
  src/blank_line.h
  src/brace_cleanup.h
  src/braces.h
  src/calculate_closing_brace_position.h
  src/can_increase_nl.h
  src/change_int_types.h
  src/char_table.h
  src/check_double_brace_init.h
  src/check_template.h
  src/chunk.h
  src/ChunkStack.h
  src/collapse_empty_body.h
  src/combine_fix_mark.h
  src/combine.h
  src/combine_labels.h
  src/combine_skip.h
  src/combine_tools.h
  src/compat.h
  src/cs_top_is_question.h
  src/detect.h
  src/do_it_newlines_func_pre_blank_lines.h
  src/double_newline.h
  src/enum_cleanup.h
  src/enum_flags.h
  src/EnumStructUnionParser.h
  src/error_types.h
  src/flag_braced_init_list.h
  src/flag_decltype.h
  src/flag_parens.h
  src/get_closing_brace.h
  src/ifdef_over_whole_file.h
  src/indent.h
  src/is_class_one_liner.h
  src/is_func_call_or_def.h
  src/is_func_proto_group.h
  src/is_var_def.h
  src/keywords.h
  src/lang_pawn.h
  src/language_names.h
  src/language_tools.h
  src/ListManager.h
  src/logger.h
  src/log_levels.h
  src/logmask.h
  src/log_rules.h
  src/mark_change.h
  src/mark_functor.h
  src/mark_question_colon.h
  src/md5.h
  src/newline_add.h
  src/newline_after.h
  src/newline_before_return.h
  src/newline_case.h
  src/newline_del_between.h
  src/newline_end_newline.h
  src/newline_force.h
  src/newline_func.h
  src/newline_iarf.h
  src/newline_min_after.h
  src/newline_oc_msg.h
  src/newlines_between.h
  src/newlines_brace_pair.h
  src/newlines_chunk_pos.h
  src/newlines_class_colon_pos.h
  src/newlines_cleanup.h
  src/newlines_cuddle_uncuddle.h
  src/newlines_do_else.h
  src/newlines_double_space_struct_enum_union.h
  src/newlines_eat_start_end.h
  src/newlines_enum.h
  src/newlines_functions_remove_extra_blank_lines.h
  src/newlines_if_for_while_switch.h
  src/newlines_if_for_while_switch_post_blank_lines.h
  src/newlines_if_for_while_switch_pre_blank_lines.h
  src/newlines_namespace.h
  src/newlines_remove_disallowed.h
  src/newlines_remove_newlines.h
  src/newlines_sparens.h
  src/newlines_squeeze_ifdef.h
  src/newlines_squeeze_paren_close.h
  src/newlines_struct_union.h
  src/newline_template.h
  src/newline_template_option.h
  src/newline_var_def_blk.h
  src/nl_create_list_liner.h
  src/nl_create_one_liner.h
  src/nl_handle_define.h
  src/one_liner_nl_ok.h
  src/option.h
  src/options_for_QT.h
  src/options.h
  src/output.h
  src/parameter_pack_cleanup.h
  src/parens.h
  src/parent_for_pp.h
  src/parsing_frame.h
  src/parsing_frame_stack.h
  src/pcf_flags.h
  src/pragma_cleanup.h
  src/prototypes.h
  src/punctuators.h
  src/quick_align_again.h
  src/reindent_line.h
  src/remove_duplicate_include.h
  src/remove_extra_returns.h
  src/remove_next_newlines.h
  src/rewrite_infinite_loops.h
  src/semicolons.h
  src/setup_newline_add.h
  src/sorting.h
  src/space.h
  src/symbols_table.h
  src/token_enum.h
  src/token_is_within_trailing_return.h
  src/tokenize_cleanup.h
  src/tokenize.h
  src/too_big_for_nl_max.h
  src/unc_ctype.h
  src/uncrustify.h
  src/uncrustify_limits.h
  src/uncrustify_types.h
  src/unc_text.h
  src/unc_tools.h
  src/undo_one_liner.h
  src/unicode.h
  src/universalindentgui.h
  src/width.h
  src/windows_compat.h
  ${PROJECT_BINARY_DIR}/src/option_enum.h
  ${PROJECT_BINARY_DIR}/uncrustify_version.h
)

set(uncrustify_docs
  "${PROJECT_SOURCE_DIR}/AUTHORS"
  "${PROJECT_SOURCE_DIR}/BUGS"
  "${PROJECT_SOURCE_DIR}/ChangeLog"
  "${PROJECT_SOURCE_DIR}/COPYING"
  "${PROJECT_SOURCE_DIR}/HELP"
  "${PROJECT_SOURCE_DIR}/README.md"
)

add_executable(uncrustify ${uncrustify_sources} ${uncrustify_headers})
add_dependencies(uncrustify generate_version_header)

set_property(TARGET uncrustify APPEND PROPERTY
  COMPILE_DEFINITIONS $<$<OR:$<CONFIG:Debug>,$<CONFIG:>>:DEBUG>
)

#
# Generate uncrustify.1
#
configure_file(man/uncrustify.1.in uncrustify.1 @ONLY)

#
# Generate uncrustify.xml (katepart highlighting file)
#
add_custom_target(katehl
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/etc/uncrustify.xml
)

#
# Tests
#
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()

#
# Coverage
#
OPTION(ENABLE_CODECOVERAGE "Enable code coverage testing support")
if(ENABLE_CODECOVERAGE)
    set(CODECOVERAGE_DEPENDS uncrustify)
    include(${CMAKE_SOURCE_DIR}/cmake/CodeCoverage.cmake)
endif(ENABLE_CODECOVERAGE)

#
# Build command to run uncrustify on its own sources
#
add_custom_target(format-sources)
foreach(source IN LISTS uncrustify_sources uncrustify_headers)
  get_filename_component(source_name  ${source} NAME)
  add_custom_target(format-${source_name}
    COMMAND uncrustify
      -c forUncrustifySources.cfg
      -lCPP --no-backup ${source}
    COMMENT "Formatting ${source}"
    WORKING_DIRECTORY ${uncrustify_SOURCE_DIR}
  )
  add_dependencies(format-sources format-${source_name})
endforeach()

#
# Package
#
set(CPACK_PACKAGE_NAME "uncrustify")
set(CPACK_PACKAGE_VERSION "${UNCRUSTIFY_VERSION}")
set(CPACK_PACKAGE_VENDOR "Ben Gardner")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Code beautifier")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
set(CPACK_SOURCE_IGNORE_FILES "/\\\\.git/;/\\\\.hg/;/tests/results/;/build.*/")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
  set(CPACK_GENERATOR "ZIP")
endif()
include(CPack)

#
# Install
#
if(MSVC)
  install(TARGETS uncrustify DESTINATION ".")
  install(FILES ${uncrustify_docs}
    DESTINATION "."
  )
  install(FILES "${PROJECT_SOURCE_DIR}/documentation/htdocs/index.html"
    DESTINATION "doc"
  )
  install(DIRECTORY "${PROJECT_SOURCE_DIR}/etc/"
    DESTINATION "cfg"
    FILES_MATCHING PATTERN "*.cfg"
  )
else()
  include(GNUInstallDirs)
  install(TARGETS uncrustify
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  )
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/uncrustify.1"
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
  )
  install(DIRECTORY "${PROJECT_SOURCE_DIR}/etc/"
    DESTINATION "${CMAKE_INSTALL_DOCDIR}/examples"
    FILES_MATCHING PATTERN "*.cfg"
  )
  install(FILES ${uncrustify_docs}
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
  )
endif()

#
# Uninstall
#
get_directory_property(hasParent PARENT_DIRECTORY)
if(NOT hasParent)
  add_custom_target(uninstall "${CMAKE_COMMAND}" -P "${PROJECT_SOURCE_DIR}/cmake/uninstall.cmake")
endif()
