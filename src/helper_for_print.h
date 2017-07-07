/**
 * @file helper_for_print.h
 *
 * Functions to help for printing with fprintf family
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef HELPER_FOR_PRINT_H_INCLUDED
#define HELPER_FOR_PRINT_H_INCLUDED

/*
 * the helper is introduced because the compiler /usr/bin/x86_64-w64-mingw32-g++
 * produces a warning for the lines:
 *            fprintf(stderr, "Unmatched BRACE_CLOSE\nat line=%zu, column=%zu\n",
 *                    pc->orig_line, pc->orig_col);
 * "unknown conversion type character ‘z’ in format [-Werror=format]"
 */

//! helper function for fprintf
char *make_message(const char *fmt, ...);

#endif /* HELPER_FOR_PRINT_H_INCLUDED */
