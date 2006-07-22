AUTOMAKE_OPTIONS = foreign no-dependencies nostdinc

bin_PROGRAMS = uncrustify

BUILT_SOURCES = token_names.h

uncrustify_SOURCES = uncrustify.cpp chunk_list.cpp indent.cpp align.cpp combine.cpp \
		tokenize.cpp tokenize_cleanup.cpp space.cpp newlines.cpp output.cpp \
		args.cpp options.cpp keywords.cpp punctuators.cpp parse_frame.cpp \
		logmask.cpp logger.cpp ChunkStack.cpp braces.cpp brace_cleanup.cpp \
		align_stack.cpp defines.cpp

noinst_HEADERS = chunk_list.h options.h char_table.h chunk_list.h \
		uncrustify_types.h prototypes.h logmask.h logger.h \
		ChunkStack.h args.h token_names.h token_enum.h ListManager.h

token_names.h: token_enum.h ../make_token_names.sh
	@echo "Rebuilding token_names.h"
	@sh ../make_token_names.sh

uncrustify_CPPFLAGS = -Wall
