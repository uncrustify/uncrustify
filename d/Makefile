#
# Simple D makefile
#
#

DCC=dmd
DCC_OPT=""
PROGRAM=uncrustify-d

.default: $(PROGRAM)

$(PROGRAM): main.d args.d
	$(DCC) $^ -of$@

