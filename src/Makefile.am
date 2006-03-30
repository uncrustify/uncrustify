AUTOMAKE_OPTIONS = foreign no-dependencies
# 1.2 gnits

bin_PROGRAMS = uncrustify
uncrustify_SOURCES = uncrustify.c chunk_list.c c_indent.c c_align.c c_combine.c \
		tokenize.c tokenize_cleanup.c c_space.c c_newlines.c c_output.c \
		args.c options.c keywords.c punctuators.c parse_frame.c \
		logmask.c logger.c chunk_stack.c braces.c brace_cleanup.c

noinst_HEADERS = chunk_list.h c_settings.h char_table.h chunk_list.h \
		cparse_types.h prototypes.h c_settings.h logmask.h logger.h \
		chunk_stack.h args.h
		
uncrustify_LDADD =  $(INTLLIBS)

# MAINTAINERCLEANFILES= Makefile.in

