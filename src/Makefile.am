AUTOMAKE_OPTIONS = foreign no-dependencies
# 1.2 gnits

bin_PROGRAMS = uncrustify
uncrustify_SOURCES = uncrustify.cpp chunk_list.cpp indent.cpp align.cpp combine.cpp \
		tokenize.cpp tokenize_cleanup.cpp space.cpp newlines.cpp output.cpp \
		args.cpp options.cpp keywords.cpp punctuators.cpp parse_frame.cpp \
		logmask.cpp logger.cpp ChunkStack.cpp braces.cpp brace_cleanup.cpp

noinst_HEADERS = chunk_list.h options.h char_table.h chunk_list.h \
		uncrustify_types.h prototypes.h logmask.h logger.h \
		ChunkStack.h args.h
		
uncrustify_LDADD =  $(INTLLIBS)

# MAINTAINERCLEANFILES= Makefile.in

