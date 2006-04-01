#
# Simple D makefile
#
#

DCC=dmd
DCC_OPT=""
PROGRAM=uncrustify-d

.default: $(PROGRAM)

$(PROGRAM): main.d args.d log.d settings.d options.d
	$(DCC) $^ -of$@

