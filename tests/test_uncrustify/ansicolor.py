# Print in color, if possible.
#
# * @author  Matthew Woehlke    June 2018
#

# Curses setup might fail...
try:
    import curses
    import sys

    curses.setupterm()

    if sys.stdout.isatty():
        def _tparm(p, *args):
            return curses.tparm(p, *args).decode('ascii')

        _setf = curses.tigetstr('setaf') or curses.tigetstr('setf')
        _setb = curses.tigetstr('setab') or curses.tigetstr('setb')
        _bold = curses.tigetstr('bold')
        _reset = _tparm(curses.tigetstr('sgr0'))

    else:
        def _tparm(p, *args):
            return ''

        _setf = ''
        _setb = ''
        _bold = ''
        _reset = ''

    # -------------------------------------------------------------------------
    def printc(ctext, ntext='', fore=None, back=None, bold=False):
        reset = ""

        if bold:
            ctext = _tparm(_bold) + ctext
            reset = _reset

        if fore is not None:
            ctext = _tparm(_setf, fore) + ctext
            reset = _reset

        if back is not None:
            ctext = _tparm(_setf, back) + ctext
            reset = _reset

        print(ctext + reset + ntext)

# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

# ...so if something went wrong, provide a fall-back instead
except Exception:
    # -------------------------------------------------------------------------
    def printc(ctext, ntext, *args, **kwargs):
        print(ctext + ntext)
