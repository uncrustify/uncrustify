/+
 *      Copyright (c) 1999-2006 by Digital Mars
 *      All Rights Reserved
 *      written by Walter Bright www.digitalmars.com
 *      License for redistribution is by either the Artistic License in artistic.txt, or the GNU General Public License in gnu.txt.
 *      See the included readme.txt for details.
 *      D Language conversion by: J Duncan
 +/

/**
 *      d language lexer
 */

module dparser.Lexer;

import dparser.Root;

//private import std.string;
//import dwf.core.debugapi;

int errno = 0;

//#if _WIN32 && __DMC__
// from \dm\src\include\setlocal.h
//extern "C" char * __cdecl __locale_decpoint;
char* __locale_decpoint;
//#endif
//const uint LS = 0x2028;	// UTF line separator

//extern int isUniAlpha(unsigned u);

/**
 *      Lexer object
 */

class Lexer
{
    static           Identifier[char[]]       stringtable;
    static OutBuffer stringbuffer;
    static Token     * freelist;

    Token            token;        // current token
    ubyte            *base;        // pointer to start of buffer
    int              doDocComment; // collect doc comment information


    this(Module mod, ubyte* base, uint begoffset, uint endoffset, int doDocComment, int commentToken)
    {
        if (stringbuffer is null) {
            stringbuffer = new OutBuffer;
        }
        loc = Loc(mod, 1);

        this.base = base;

        /*
         *      If first line starts with '#!', ignore the line
         */

        if (p[0] == '#' && p[1] == '!') {
            p += 2;
            while (true) {
                ubyte c = *p;
                switch (c) {
                case '\n':
                    p++;
                    break;

                default:
                    if (c & 0x80) {
                        uint u = decodeUTF();
                        if (u == PS || u == LS) {
                            break;
                        }
                    }
                    p++;
                    continue;
                }
                break;
            }

            loc.linnum = 2;
        }
    }



    // generate a unique identifier for this string
    static Identifier idPool(in char[] str) {
//	    StringValue sv;
        if ((str in stringtable) == null) {
            stringtable[str] = new Identifier(str, TOK.TOKidentifier);
        }
        return(stringtable[str]);
    }

    void error(...) {
        if ((mod !is null) && !global.gag) {
            writefln(formatLoc(loc, _arguments, _argptr));
            /*
             * char[] p = loc.toChars();
             */
            if (global.errors >= global.max_errors) {                   // moderate blizzard of cascading messages
                throw new Exception("too many errors");
            }
        }

        global.errors++;
    }

    Token* peek(inout Token ct) {
        Token* t;

        if (ct.next) {
            t = ct.next;
        }
        else {
            ct.next = t;
        }
        return(t);
    }

    /**************************************
     * Lex hex strings:
     *	x"0A ae 34FE BD"
     */

    TOK hexStringConstant(Token *t) {
        uint c;
        Loc  start = loc;

        p++;
        stringbuffer.offset = 0;
        while (1) {
            c = *p++;
            switch (c) {
            case ' ':
            }
        }
    }

    /***************************************
     * Read \u or \U unicode sequence
     */
    /*
     * uint Wchar(uint u)
     * {
     *  uint value;
     *  return value;
     * }
     */


    /*********************************************
     * Do pragma.
     * Currently, the only pragma supported is:
     *	#line linnum [filespec]
     */

    void Pragma() {
        Token  tok;
        int    linnum;
        char[] filespec;
        Loc    loc = this.loc;

        scan(&tok);

        if (tok.value != TOK.TOKidentifier || tok.identifier != Id.line) {
            goto Lerr;
        }

        scan(&tok);
        if (tok.value == TOK.TOKint32v || tok.value == TOK.TOKint64v) {
            linnum = tok.uns64value - 1;
        }
        else {
            goto Lerr;
        }

Lerr:
        errorLoc(loc, "#line integer [\"filespec\"]\\n expected");
    }
}

// character maps
static ubyte[256] cmtable;

const int CMoctal = 0x1;

ubyte isoctal(ubyte c) {
    return(cmtable[c] & CMoctal);
}

static void cmtable_init() {
    for (uint c = 0; c < cmtable.length; c++) {
        if (isdigit(c) || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F')) {
            cmtable[c] |= CMhex;
        }
    }
}
