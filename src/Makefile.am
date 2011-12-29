AUTOMAKE_OPTIONS = foreign no-dependencies

DISTCLEANFILES =
CLEANFILES = *.o *~ *.bak
MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = uncrustify

BUILT_SOURCES = token_names.h

uncrustify_SOURCES = uncrustify.cpp chunk_list.cpp indent.cpp align.cpp combine.cpp \
		tokenize.cpp tokenize_cleanup.cpp space.cpp newlines.cpp output.cpp \
		args.cpp options.cpp keywords.cpp punctuators.cpp parse_frame.cpp \
		logmask.cpp logger.cpp ChunkStack.cpp braces.cpp brace_cleanup.cpp \
		align_stack.cpp defines.cpp width.cpp lang_pawn.cpp md5.cpp \
		backup.cpp parens.cpp universalindentgui.cpp semicolons.cpp \
		sorting.cpp detect.cpp unicode.cpp unc_text.cpp \
		compat_posix.cpp compat_win32.cpp

noinst_HEADERS = chunk_list.h options.h char_table.h chunk_list.h \
		uncrustify_types.h prototypes.h logmask.h logger.h \
		ChunkStack.h args.h token_names.h token_enum.h ListManager.h md5.h \
		align_stack.h backup.h base_types.h log_levels.h \
		punctuators.h \
		uncrustify_version.h \
		unc_ctype.h unc_text.h

token_names.h: token_enum.h ../make_token_names.sh
	@echo "Rebuilding token_names.h"
	@sh ../make_token_names.sh

uncrustify_CPPFLAGS = -Wall
