#! /usr/bin/env python
# tokenize.py
#
# Parses a C/C++/C#/D/Java/Pawn/whatever file in an array of
# tuples (string, type)
#

# punctuator lookup table
punc_table = [
   [ '!',  25,  26, '!'   ],   #   0: '!'
   [ '#',  24,  35, '#'   ],   #   1: '#'
   [ '$',  23,   0, '$'   ],   #   2: '$'
   [ '%',  22,  36, '%'   ],   #   3: '%'
   [ '&',  21,  41, '&'   ],   #   4: '&'
   [ '(',  20,   0, '('   ],   #   5: '('
   [ ')',  19,   0, ')'   ],   #   6: ')'
   [ '*',  18,  43, '*'   ],   #   7: '*'
   [ '+',  17,  44, '+'   ],   #   8: '+'
   [ ',',  16,   0, ','   ],   #   9: ','
   [ '-',  15,  46, '-'   ],   #  10: '-'
   [ '.',  14,  50, '.'   ],   #  11: '.'
   [ '/',  13,  53, '/'   ],   #  12: '/'
   [ ':',  12,  54, ':'   ],   #  13: ':'
   [ ';',  11,   0, ';'   ],   #  14: ';'
   [ '<',  10,  56, '<'   ],   #  15: '<'
   [ '=',   9,  63, '='   ],   #  16: '='
   [ '>',   8,  65, '>'   ],   #  17: '>'
   [ '?',   7,   0, '?'   ],   #  18: '?'
   [ '[',   6,  70, '['   ],   #  19: '['
   [ ']',   5,   0, ']'   ],   #  20: ']'
   [ '^',   4,  71, '^'   ],   #  21: '^'
   [ '{',   3,   0, '{'   ],   #  22: '{'
   [ '|',   2,  72, '|'   ],   #  23: '|'
   [ '}',   1,   0, '}'   ],   #  24: '}'
   [ '~',   0,  74, '~'   ],   #  25: '~'
   [ '<',   3,  30, '!<'  ],   #  26: '!<'
   [ '=',   2,  33, '!='  ],   #  27: '!='
   [ '>',   1,  34, '!>'  ],   #  28: '!>'
   [ '~',   0,   0, '!~'  ],   #  29: '!~'
   [ '=',   1,   0, '!<=' ],   #  30: '!<='
   [ '>',   0,  32, '!<>' ],   #  31: '!<>'
   [ '=',   0,   0, '!<>='],   #  32: '!<>='
   [ '=',   0,   0, '!==' ],   #  33: '!=='
   [ '=',   0,   0, '!>=' ],   #  34: '!>='
   [ '#',   0,   0, '##'  ],   #  35: '##'
   [ ':',   2,  39, '%:'  ],   #  36: '%:'
   [ '=',   1,   0, '%='  ],   #  37: '%='
   [ '>',   0,   0, '%>'  ],   #  38: '%>'
   [ '%',   0,  40, None  ],   #  39: '%:%'
   [ ':',   0,   0, '%:%:'],   #  40: '%:%:'
   [ '&',   1,   0, '&&'  ],   #  41: '&&'
   [ '=',   0,   0, '&='  ],   #  42: '&='
   [ '=',   0,   0, '*='  ],   #  43: '*='
   [ '+',   1,   0, '++'  ],   #  44: '++'
   [ '=',   0,   0, '+='  ],   #  45: '+='
   [ '-',   2,   0, '--'  ],   #  46: '--'
   [ '=',   1,   0, '-='  ],   #  47: '-='
   [ '>',   0,  49, '->'  ],   #  48: '->'
   [ '*',   0,   0, '->*' ],   #  49: '->*'
   [ '*',   1,   0, '.*'  ],   #  50: '.*'
   [ '.',   0,  52, '..'  ],   #  51: '..'
   [ '.',   0,   0, '...' ],   #  52: '...'
   [ '=',   0,   0, '/='  ],   #  53: '/='
   [ ':',   1,   0, '::'  ],   #  54: '::'
   [ '>',   0,   0, ':>'  ],   #  55: ':>'
   [ '%',   4,   0, '<%'  ],   #  56: '<%'
   [ ':',   3,   0, '<:'  ],   #  57: '<:'
   [ '<',   2,  61, '<<'  ],   #  58: '<<'
   [ '=',   1,   0, '<='  ],   #  59: '<='
   [ '>',   0,  62, '<>'  ],   #  60: '<>'
   [ '=',   0,   0, '<<=' ],   #  61: '<<='
   [ '=',   0,   0, '<>=' ],   #  62: '<>='
   [ '=',   0,  64, '=='  ],   #  63: '=='
   [ '=',   0,   0, '===' ],   #  64: '==='
   [ '=',   1,   0, '>='  ],   #  65: '>='
   [ '>',   0,  67, '>>'  ],   #  66: '>>'
   [ '=',   1,   0, '>>=' ],   #  67: '>>='
   [ '>',   0,  69, '>>>' ],   #  68: '>>>'
   [ '=',   0,   0, '>>>='],   #  69: '>>>='
   [ ']',   0,   0, '[]'  ],   #  70: '[]'
   [ '=',   0,   0, '^='  ],   #  71: '^='
   [ '=',   1,   0, '|='  ],   #  72: '|='
   [ '|',   0,   0, '||'  ],   #  73: '||'
   [ '=',   1,   0, '~='  ],   #  74: '~='
   [ '~',   0,   0, '~~'  ],   #  75: '~~'
]


#
# Token types:
#  0 = newline
#  1 = punctuator
#  2 = integer
#  3 = float
#  4 = string
#  5 = identifier
#
class Tokenizer:
    def __init__(self):
        self.tokens = []
        self.text = ''
        self.text_idx = 0

    def tokenize_text(self, in_text):
        self.tokens = []
        self.text = in_text
        self.text_idx = 0

        print(in_text)
        try:
            while self.text_idx < len(self.text):
                if self.parse_whitespace():
                    continue
                elif self.text[self.text_idx] == '\\' and self.text[self.text_idx + 1] == '\n':
                    self.text_idx += 2
                    continue
                elif self.parse_comment():
                    continue
                elif self.parse_number():
                    continue
                elif self.parse_identifier():
                    continue
                elif self.parse_string():
                    continue
                elif self.parse_punctuator():
                    continue
                else:
                    print("confused: %s" % self.text[self.text_idx:])
                    break
        except:
            print("bombed")
            raise

    def parse_whitespace(self):
        start_idx = self.text_idx
        hit_newline = False
        while self.text_idx < len(self.text):
            if self.text[self.text_idx] in '\n\r':
                hit_newline = True
            elif not self.text[self.text_idx] in ' \t':
                break
            self.text_idx += 1

        if hit_newline:
            self.tokens.append(('\n', 0))
        return start_idx != self.text_idx

    def parse_comment(self):
        if not self.text[self.text_idx] == '/' or not self.text[self.text_idx + 1] in '/*':
            return False
        if self.text[self.text_idx + 1] == '/':
            while self.text_idx < len(self.text):
                if self.text[self.text_idx] in '\n\r':
                    break
                self.text_idx += 1
        else:
            while self.text_idx < len(self.text) - 1:
                if self.text[self.text_idx] == '*' and self.text[self.text_idx + 1] == '/':
                    self.text_idx += 2
                    break
                self.text_idx += 1
        return True

    def parse_identifier(self):
        if not self.text[self.text_idx].upper() in '@_ABCDEFGHIJKLMNOPQRSTUVWXYZ':
            return False
        start_idx = self.text_idx
        while self.text_idx < len(self.text) and \
                self.text[self.text_idx].upper() in '@_ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890':
            self.text_idx += 1
        self.tokens.append((self.text[start_idx : self.text_idx], 5))
        return True

    def parse_string(self):
        starter = 0
        start_ch = self.text[self.text_idx]
        if start_ch == 'L':
            starter = 1
            start_ch = self.text[self.text_idx + 1]
        if not start_ch in '"\'':
            return False
        start_idx = self.text_idx
        self.text_idx += starter + 1
        escaped = False
        while self.text_idx < len(self.text):
            if escaped:
                escaped = False
            else:
                if self.text[self.text_idx] == '\\':
                    escaped = True
                elif self.text[self.text_idx] == start_ch:
                    self.text_idx += 1
                    break
            self.text_idx += 1

        self.tokens.append((self.text[start_idx : self.text_idx], 4))
        return True

    # Checks for punctuators
    # Returns whether a punctuator was consumed (True or False)
    def parse_punctuator(self):
        tab_idx = 0
        punc_len = 0
        saved_punc = None
        while 1:
            pte = punc_table[tab_idx]
            if pte[0] == self.text[self.text_idx]:
                if pte[3] is not None:
                    saved_punc = pte[3]
                self.text_idx += 1
                tab_idx = pte[2]
                if tab_idx == 0:
                    break
            elif pte[1] == 0:
                break
            else:
                tab_idx += 1
        if saved_punc is not None:
            self.tokens.append((saved_punc, 1))
            return True
        return False

    def parse_number(self):
        # A number must start with a digit or a dot followed by a digit
        ch = self.text[self.text_idx]
        if not ch.isdigit() and (ch != '.' or not self.text[self.text_idx + 1].isdigit()):
            return False
        token_type = 2 # integer
        if ch == '.':
            token_type = 3 # float
        did_hex = False
        start_idx = self.text_idx

        # Check for Hex, Octal, or Binary
        # Note that only D and Pawn support binary, but who cares?
        #
        if ch == '0':
            self.text_idx += 1
            ch = self.text[self.text_idx].upper()
            if ch == 'X':                # hex
                did_hex = True
                self.text_idx += 1
                while self.text[self.text_idx] in '_0123456789abcdefABCDEF':
                    self.text_idx += 1
            elif ch == 'B':              # binary
                self.text_idx += 1
                while self.text[self.text_idx] in '_01':
                    self.text_idx += 1
            elif ch >= '0' and ch <= 7:  # octal (but allow decimal)
                self.text_idx += 1
                while self.text[self.text_idx] in '_0123456789':
                    self.text_idx += 1
            else:
                # either just 0 or 0.1 or 0UL, etc
                pass
        else:
            # Regular int or float
            while self.text[self.text_idx] in '_0123456789':
                self.text_idx += 1

        # Check if we stopped on a decimal point
        if self.text[self.text_idx] == '.':
            self.text_idx += 1
            token_type = 3 # float
            if did_hex:
                while self.text[self.text_idx] in '_0123456789abcdefABCDEF':
                    self.text_idx += 1
            else:
                while self.text[self.text_idx] in '_0123456789':
                    self.text_idx += 1

        # Check exponent
        # Valid exponents per language (not that it matters):
        # C/C++/D/Java: eEpP
        # C#/Pawn:      eE
        if self.text[self.text_idx] in 'eEpP':
            token_type = 3 # float
            self.text_idx += 1
            if self.text[self.text_idx] in '+-':
                self.text_idx += 1
            while self.text[self.text_idx] in '_0123456789':
                self.text_idx += 1

        # Check the suffixes
        # Valid suffixes per language (not that it matters):
        #        Integer       Float
        # C/C++: uUlL          lLfF
        # C#:    uUlL          fFdDMm
        # D:     uUL           ifFL
        # Java:  lL            fFdD
        # Pawn:  (none)        (none)
        #
        # Note that i, f, d, and m only appear in floats.
        while 1:
            if self.text[self.text_idx] in 'tTfFdDmM':
                token_type = 3 # float
            elif not self.text[self.text_idx] in 'lLuU':
                break
            self.text_idx += 1

        self.tokens.append((self.text[start_idx : self.text_idx], token_type))
        return True

text = """
1.23+4-3*16%2 *sin(1.e-3 + .5p32) "hello" and "hello\\"there"
123 // some comment
a = b + c;
#define abc \\
        5
d = 5 /* hello */ + 3;
"""

t = Tokenizer()
t.tokenize_text(text)
print(t.tokens)

