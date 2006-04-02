#
# Simple D makefile
#
#

DCC=dmd
DCC_OPT=""
PROGRAM=uncrustify-d
SOURCES=main.d args.d log.d Logger.d LogMask.d LogSev.d settings.d options.d \
        TokenEnum.d TokenName.d

.default: $(PROGRAM)

$(PROGRAM): $(SOURCES)
	$(DCC) $^ -of$@

