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

import dparser.Tokens;
import dparser.Token;
import dparser.Keyword;

import dparser.Types;

import dparser.Module;
import dparser.Identifier;
import dparser.unialpha;

import dparser.OutBuffer;

//private import std.ctype;
//private import std.string;
//import dwf.core.debugapi;

int errno = 0;

//#if _WIN32 && __DMC__
// from \dm\src\include\setlocal.h
//extern "C" char * __cdecl __locale_decpoint;
char* __locale_decpoint;
//#endif
//const uint LS = 0x2028;	// UTF line separator
//const uint PS = 0x2029;	// UTF paragraph separator

//extern int isUniAlpha(unsigned u);
//extern int HtmlNamedEntity(unsigned char *p, int length);

/**
 *      Lexer object
 */

class Lexer
{
    static           Identifier[char[]]       stringtable;
    static OutBuffer stringbuffer;
    static Token     * freelist;

    Token            token;        // current token
    Module           mod;          // current module
    Loc              loc;          // for error messages
    ubyte            *base;        // pointer to start of buffer
    ubyte            *end;         // past end of buffer
    ubyte            *p;           // current character
    int              doDocComment; // collect doc comment information
    int              anyToken;     // !=0 means seen at least one token
    int              commentToken; // !=0 means comments are TOKcomment's


    this(Module mod, ubyte* base, uint begoffset, uint endoffset, int doDocComment, int commentToken)
    {
        if (stringbuffer is null) {
            stringbuffer = new OutBuffer;
        }
        loc = Loc(mod, 1);

        this.base         = base;
        this.end          = base + endoffset;
        this.p            = base + begoffset;
        this.mod          = mod;
        this.doDocComment = doDocComment;
        this.commentToken = commentToken;

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

                case '\r':
                    p++;
                    if (*p == '\n') {
                        p++;
                    }
                    break;

                case 0:
                case 0x1A:
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
//	    uint len = s.length;
//	    StringValue sv = stringtable.update(s, len);
//	    Identifier* id = cast(Identifier*) sv.ptrvalue;
//	    if( id is null )
        if ((str in stringtable) == null) {
            stringtable[str] = new Identifier(str, TOK.TOKidentifier);
        }
        return(stringtable[str]);
    }

    static void initKeywords() {
        // build character map
        cmtable_init();

        // create keyword tokens & identifiers
        dparser.Keyword.initKeywords();

        // create standard lexer tokens
        dparser.Token.createLexerTokens();
    }

    // Combine two document comments into one.
    static char[] combineComments(char[] c1, char[] c2) {
        char[] c = c2;
        if (c1.length) {
            c = c1;
            if (c2.length) {
                c = c1 ~ "\n" ~ c2;
            }
        }
        return(c);
    }

    // Decode UTF character. Issue error messages for invalid sequences. Return decoded character, advance p to last character in UTF sequence.
    //! fix
    uint decodeUTF() {
        ubyte * s = p;
        ubyte c   = *s;

        assert(c & 0x80);
        if (!(c & 0x80)) {
            return(c);
        }

        return(cast(uint)'X');
        /*
         *  dchar u;
         *  uint len;
         *
         *
         *
         *  // Check length of remaining string up to 6 UTF-8 characters
         *  for( len = 1; len < 6 && s[len]; len++ )
         *  {
         *
         *  }
         *              /+
         *  uint idx = 0;
         *  char* msg = utf_decodeChar( s, len, &idx, &u );
         *  p += idx - 1;
         *  if( msg )
         *  {
         *              error(msg);
         *  }
         * +/
         *  return u;
         */
    }

    void error(...) {
        if ((mod !is null) && !global.gag) {
            writefln(formatLoc(loc, _arguments, _argptr));
            /*
             * char[] p = loc.toChars();
             * if( p.length )
             *  writef( "%s: ", p );
             * writefx( stdout, _arguments, _argptr, 1 );
             */
            if (global.errors >= global.max_errors) {                   // moderate blizzard of cascading messages
                throw new Exception("too many errors");
            }
        }

        global.errors++;
    }

    void errorLoc(Loc loc, ...) {
        if ((mod !is null) && !global.gag) {
            writefln(formatLoc(loc, _arguments, _argptr));
            /*
             * char[] p = loc.toChars();
             * if( p.length )
             *  writef("%s: ", p);
             * writefx(stdout, _arguments, _argptr, 1);
             */
            if (global.errors >= 20) {                  // moderate blizzard of cascading messages
                throw new Exception("too many errors");
            }
        }

        global.errors++;
    }


    TOK nextToken() {
        if (token.next) {
            Token* t = token.next;
            memcpy(&token, t, Token.sizeof);
//			t.next = freelist;
//			freelist = t;
        }
        else {
            scan(&token);
        }
//	    token.print();
        return(token.value);
    }

    Token* peek(inout Token ct) {
        Token* t;

        if (ct.next) {
            t = ct.next;
        }
        else {
            t = new Token;
            scan(t);
            t.next  = null;
            ct.next = t;
        }
        return(t);
    }

    // Turn next token in buffer into a token.

    void scan(Token* t) {
//		debug writefln("scan token");
        uint lastLine = loc.linnum;
        uint linnum;

        t.blockComment = null;
        t.lineComment  = null;
        while (true) {
            t.ptr = p;
//			debug writefln( "    p = %d, *p = ", cast(uint)p, cast(char)*p );
            switch (*p) {
            case 0:
            case 0x1a:
                t.value = TOK.TOKeof;                                           // end of file
//					debug writefln( "    EOF" );
                return;

            case ' ':
            case '\t':
            case '\v':
            case '\f':
                p++;
//					debug writefln( "    whitespace" );
                continue;                                                               // skip white space

            case '\r':
//					debug writefln( "    cr" );
                p++;
                if (*p != '\n') {                                               // if CR stands by itself
                    loc.linnum++;
                }
                continue;                                                               // skip white space

            case '\n':
//					debug writefln( "    nl" );
                p++;
                loc.linnum++;
                continue;                                                               // skip white space

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                t.value = number(t);
                return;

/*
 * #if CSTRINGS
 *                          case '\'':
 *                              t.value = charConstant(t, 0);
 *                              return;
 *
 *                          case '"':
 *                              t.value = stringConstant(t,0);
 *                              return;
 *
 *                          case 'l':
 *                          case 'L':
 *                              if( p[1] == '\'')
 *                              {
 *                                  p++;
 *                                  t.value = charConstant(t, 1);
 *                                  return;
 *                              }
 *                              else if( p[1] == '"')
 *                              {
 *                                  p++;
 *                                  t.value = stringConstant(t, 1);
 *                                  return;
 *                              }
 * #else
 */
            case '\'':
//					debug writefln( "    char" );
                t.value = charConstant(t, 0);
                return;

            case 'r':
//					debug writefln( "    wysiwyg" );
                if (p[1] != '"') {
                    goto case_identifier;
                }
                p++;

            case '`':
                t.value = wysiwygStringConstant(t, *p);
                return;

            case 'x':
//					debug writefln( "    hex string" );
                if (p[1] != '"') {
                    goto case_identifier;
                }
                p++;
                t.value = hexStringConstant(t);
                return;


            case '"':
//					debug writefln( "    string" );
                t.value = escapeStringConstant(t, 0);
//					debug writefln( t.ustring );
                return;

            case '\\':                                  // escaped string literal
//					debug writefln( "    escaped string literal" );
                uint c;
                stringbuffer.offset = 0;
                do {
                    p++;
                    c = escapeSequence();
                    stringbuffer.write(c);
                } while (*p == '\\');
//					t.len = stringbuffer.offset;
//					stringbuffer.write(cast(byte)0);
                t.ustring = stringbuffer.toString;
//					memcpy( t.ustring.ptr, stringbuffer.data, stringbuffer.offset );
                t.postfix = 0;
                t.value   = TOK.TOKstring;
                return;

            case 'l':
            case 'L':
//	#endif

            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
            case 'g':
            case 'h':
            case 'i':
            case 'j':
            case 'k':
            case 'm':
            case 'n':
            case 'o':
            case 'p':
            case 'q':                             /*case 'r':*/
            case 's':
            case 't':
            case 'u':
            case 'v':
            case 'w':                                         /*case 'x':*/
            case 'y':
            case 'z':
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
            case 'G':
            case 'H':
            case 'I':
            case 'J':
            case 'K':
            case 'M':
            case 'N':
            case 'O':
            case 'P':
            case 'Q':
            case 'R':
            case 'S':
            case 'T':
            case 'U':
            case 'V':
            case 'W':
            case 'X':
            case 'Y':
            case 'Z':
            case '_':
case_identifier:
                {
//					debug writefln( "    identifier" );
                    ubyte c;
                    do {
                        c = *++p;
                    } while (isidchar(c) || (c & 0x80 && isUniAlpha(decodeUTF())));

//					sv = stringtable.update((char *)t.ptr, p - t.ptr);
                    char[] tmp;
                    tmp.length = p - t.ptr;
                    memcpy(tmp.ptr, t.ptr, p - t.ptr);
                    Identifier id;
                    Identifier * pid = tmp in stringtable;
                    if (pid) {
                        id = *pid;
                    }

                    if (id is null) {
                        id               = new Identifier(tmp, TOK.TOKidentifier);
                        stringtable[tmp] = id;
                    }

                    t.identifier = id;
                    t.value      = cast(TOK)id.value;
                    anyToken     = 1;

                    // if special identifier token
                    if (*t.ptr == '_') {
                        static char date[11 + 1];
                        static char time[8 + 1];
                        static char timestamp[24 + 1];

                        if (!date[0]) {                         // lazy evaluation
                            //!!
                            /+
                             *      time_t t;
                             *      char *p;
                             *      .time(&t);
                             *      p = ctime(&t);
                             *      assert(p);
                             *      sprintf(date.ptr, "%.6s %.4s", p + 4, p + 20);
                             *      sprintf(time.ptr, "%.8s", p + 11);
                             *      sprintf(timestamp.ptr, "%.24s", p);
                             +/
                        }

                        if (mod && id is Id.FILE) {
                            t.value = TOK.TOKstring;
                            if (loc.filename.length) {
                                t.ustring = loc.filename;
                            }
                            else {
                                t.ustring = mod.identifier.toChars();
                            }
                            goto Llen;
                        }
                        else if (mod && id == Id.LINE) {
                            t.value      = TOK.TOKint64v;
                            t.uns64value = loc.linnum;
                        }
                        else if (id == Id.DATE) {
                            t.value = TOK.TOKstring;
                            //! t.ustring = date;
                            goto Llen;
                        }
                        else if (id == Id.TIME) {
                            t.value = TOK.TOKstring;
                            //! t.ustring = time;
                            goto Llen;
                        }
                        else if (id == Id.TIMESTAMP) {
                            t.value = TOK.TOKstring;
                            //! t.ustring = timestamp;
Llen:
                            t.postfix = 0;
//							t.len = strlen((char *)t.ustring);
                        }
                    }
                    //printf("t.value = %d\n",t.value);
                    return;
                }

            // comments
            case '/':
                p++;
                switch (*p) {
                case '=':
                    p++;
                    t.value = TOK.TOKdivass;
                    return;

                case '*':                               // '/*'
                    p++;
                    linnum = loc.linnum;
                    while (true) {
                        while (true) {
                            ubyte c = *p;
                            switch (c) {
                            case '/':
                                break;

                            case '\n':
                                loc.linnum++;
                                p++;
                                continue;

                            case '\r':
                                p++;
                                if (*p != '\n') {
                                    loc.linnum++;
                                }
                                continue;

                            case 0:
                            case 0x1A:
                                error("unterminated /* */ comment");
                                p       = end;
                                t.value = TOK.TOKeof;
                                return;

                            default:
                                if (c & 0x80) {
                                    uint u = decodeUTF();
                                    if (u == PS || u == LS) {
                                        loc.linnum++;
                                    }
                                }
                                p++;
                                continue;
                            }
                            break;
                        }
                        p++;
                        if (p[-2] == '*' && p - 3 != t.ptr) {
                            break;
                        }
                    }

                    if (commentToken) {
                        t.value = TOK.TOKcomment;
                        return;
                    }
                    // if /** but not /**/
                    else if (doDocComment && t.ptr[2] == '*' && p - 4 != t.ptr) {
                        getDocComment(t, lastLine == linnum);                                           //! ?
                    }
                    continue;

                case '/':                                       // do // style comments
                    linnum = loc.linnum;
                    while (1) {
                        ubyte c = *++p;
                        switch (c) {
                        case '\n':
                            break;

                        case '\r':
                            if (p[1] == '\n') {
                                p++;
                            }
                            break;

                        case 0:
                        case 0x1a:
                            if (commentToken) {
                                p       = end;
                                t.value = TOK.TOKcomment;
                                return;
                            }
                            if (doDocComment && t.ptr[2] == '/') {
                                getDocComment(t, lastLine == linnum);
                            }
                            p       = end;
                            t.value = TOK.TOKeof;
                            return;

                        default:
                            if (c & 0x80) {
                                uint u = decodeUTF();
                                if (u == PS || u == LS) {
                                    break;
                                }
                            }
                            continue;
                        }
                        break;
                    }

                    if (commentToken) {
                        p++;
                        loc.linnum++;
                        t.value = TOK.TOKcomment;
                        return;
                    }
                    if (doDocComment && t.ptr[2] == '/') {
                        getDocComment(t, lastLine == linnum);
                    }

                    p++;
                    loc.linnum++;
                    continue;

                case '+':
                    {
                        int nest;
                        linnum = loc.linnum;
                        p++;
                        nest = 1;
                        while (1) {
                            ubyte c = *p;
                            switch (c) {
                            case '/':
                                p++;
                                if (*p == '+') {
                                    p++;
                                    nest++;
                                }
                                continue;

                            case '+':
                                p++;
                                if (*p == '/') {
                                    p++;
                                    if (--nest == 0) {
                                        break;
                                    }
                                }
                                continue;

                            case '\r':
                                p++;
                                if (*p != '\n') {
                                    loc.linnum++;
                                }
                                continue;

                            case '\n':
                                loc.linnum++;
                                p++;
                                continue;

                            case 0:
                            case 0x1A:
                                error("unterminated /+ +/ comment");
                                p       = end;
                                t.value = TOK.TOKeof;
                                return;

                            default:
                                if (c & 0x80) {
                                    uint u = decodeUTF();
                                    if (u == PS || u == LS) {
                                        loc.linnum++;
                                    }
                                }
                                p++;
                                continue;
                            }
                            break;
                        }
                        if (commentToken) {
                            t.value = TOK.TOKcomment;
                            return;
                        }
                        if (doDocComment && t.ptr[2] == '+' && p - 4 != t.ptr) {
                            // if /++ but not /++/
                            getDocComment(t, lastLine == linnum);
                        }
                        continue;
                    }

                default:
                    break;
                }
                t.value = TOK.TOKdiv;
                return;

            case '.':
                p++;
                if (isdigit(*p)) {
                    p--;
                    t.value = inreal(t);
                }
                else if (p[0] == '.') {
                    if (p[1] == '.') {
                        p      += 2;
                        t.value = TOK.TOKdotdotdot;
                    }
                    else {
                        p++;
                        t.value = TOK.TOKslice;
                    }
                }
                else {
                    t.value = TOK.TOKdot;
                }
                return;

            case '&':
                p++;
                if (*p == '=') {
                    p++;
                    t.value = TOK.TOKandass;
                }
                else if (*p == '&') {
                    p++;
                    t.value = TOK.TOKandand;
                }
                else {
                    t.value = TOK.TOKand;
                }
                return;

            // |, ||, |=
            case '|':
                p++;
                if (*p == '=') {
                    p++;
                    t.value = TOK.TOKorass;
                }
                else if (*p == '|') {
                    p++;
                    t.value = TOK.TOKoror;
                }
                else {
                    t.value = TOK.TOKor;
                }
                return;

            case '-':
                p++;
                if (*p == '=') {
                    p++;
                    t.value = TOK.TOKminass;
                }
                else if (*p == '-') {
                    p++;
                    t.value = TOK.TOKminusminus;
                }
                else {
                    t.value = TOK.TOKmin;
                }
                return;

            // +, +=, ++
            case '+':
                p++;
                if (*p == '=') {
                    p++;
                    t.value = TOK.TOKaddass;                                    // +=
                }
                else if (*p == '+') {
                    p++;
                    t.value = TOK.TOKplusplus;                                          // ++
                }
                else {
                    t.value = TOK.TOKadd;                                                       // +
                }
                return;

            // <, <=, <<=, <<, <>=, <>
            case '<':
                p++;
                if (*p == '=') {
                    p++;
                    t.value = TOK.TOKle;                                                // <=
                }
                else if (*p == '<') {
                    p++;
                    if (*p == '=') {
                        p++;
                        t.value = TOK.TOKshlass;                                        // <<=
                    }
                    else {
                        t.value = TOK.TOKshl;                                           // <<
                    }
                }
                else if (*p == '>') {
                    p++;
                    if (*p == '=') {
                        p++;
                        t.value = TOK.TOKleg;                                           // <>=
                    }
                    else {
                        t.value = TOK.TOKlg;                                            // <>
                    }
                }
                else {
                    t.value = TOK.TOKlt;                                                // <
                }
                return;

            // >, >>, >>>, >=, >>=, >>>=
            case '>':
                p++;
                if (*p == '=') {
                    p++;
                    t.value = TOK.TOKge;                                                // >=
                }
                else if (*p == '>') {
                    p++;
                    if (*p == '=') {
                        p++;
                        t.value = TOK.TOKshrass;                                        // >>=
                    }
                    else if (*p == '>') {
                        p++;
                        if (*p == '=') {
                            p++;
                            t.value = TOK.TOKushrass;                           // >>>=
                        }
                        else {
                            t.value = TOK.TOKushr;                                      // >>>
                        }
                    }
                    else {
                        t.value = TOK.TOKshr;                                           // >>
                    }
                }
                else {
                    t.value = TOK.TOKgt;                                                // >
                }
                return;

            case '!':
                p++;
                if (*p == '=') {
                    p++;
                    if (*p == '=') {
                        p++;
                        t.value = TOK.TOKnotidentity;                                   // !==
                    }
                    else {
                        t.value = TOK.TOKnotequal;                                      // !=
                    }
                }
                else if (*p == '<') {
                    p++;
                    if (*p == '>') {
                        p++;
                        if (*p == '=') {
                            p++;
                            t.value = TOK.TOKunord;                             // !<>=
                        }
                        else {
                            t.value = TOK.TOKue;                                // !<>
                        }
                    }
                    else if (*p == '=') {
                        p++;
                        t.value = TOK.TOKug;                                    // !<=
                    }
                    else {
                        t.value = TOK.TOKuge;                                   // !<
                    }
                }
                else if (*p == '>') {
                    p++;
                    if (*p == '=') {
                        p++;
                        t.value = TOK.TOKul;                                    // !>=
                    }
                    else {
                        t.value = TOK.TOKule;                                   // !>
                    }
                }
                else {
                    t.value = TOK.TOKnot;                                       // !
                }
                return;

            case '=':
                p++;
                if (*p == '=') {
                    p++;
                    if (*p == '=') {
                        p++;
                        t.value = TOK.TOKidentity;                                      // ===
                    }
                    else {
                        t.value = TOK.TOKequal;                                         // ==
                    }
                }
                else {
                    t.value = TOK.TOKassign;                                    // =
                }
                return;

            case '~':
                p++;
                if (*p == '=') {
                    p++;
                    t.value = TOK.TOKcatass;                                            // ~=
                }
                else {
                    t.value = TOK.TOKtilde;                                             // ~
                }
                return;

            // SINGLE
            case '(': p++; t.value = TOK.TOKlparen;     return;

            case ')': p++; t.value = TOK.TOKrparen;     return;

            case '[': p++; t.value = TOK.TOKlbracket;   return;

            case ']': p++; t.value = TOK.TOKrbracket;   return;

            case '{': p++; t.value = TOK.TOKlcurly;     return;

            case '}': p++; t.value = TOK.TOKrcurly;     return;

            case '?': p++; t.value = TOK.TOKquestion;   return;

            case ',': p++; t.value = TOK.TOKcomma;              return;

            case ';': p++; t.value = TOK.TOKsemicolon;  return;

            case ':': p++; t.value = TOK.TOKcolon;              return;

            case '$': p++; t.value = TOK.TOKdollar;     return;

            // DOUBLE
            case '*': p++; if (*p == '=') {
                    p++; t.value = TOK.TOKmulass;
            }
                else {
                    t.value = TOK.TOKmul;
                } return;

            case '%': p++; if (*p == '=') {
                    p++; t.value = TOK.TOKmodass;
            }
                else {
                    t.value = TOK.TOKmod;
                } return;

            case '^': p++; if (*p == '=') {
                    p++; t.value = TOK.TOKxorass;
            }
                else {
                    t.value = TOK.TOKxor;
                } return;

// removed 148	case '~': p++; if( *p == '=' ) { p++; t.value = TOK.TOKcatass; } else t.value = TOK.TOKtilde; return;


            case '#':
                p++;
                Pragma();
                continue;

            default:
                {
                    debug writefln("    default char");
                    ubyte c = *p;
                    if (c & 0x80) {
                        uint u = decodeUTF();
                        // Check for start of unicode identifier
                        if (isUniAlpha(u)) {
                            goto case_identifier;
                        }

                        if (u == PS || u == LS) {
                            loc.linnum++;
                            p++;
                            continue;
                        }
                    }
                    if (isprint(c)) {
                        error("unsupported char '%s'", cast(char)c);
                    }
                    else {
                        error("unsupported char 0x%02x", cast(ubyte)c);
                    }
                    p++;
                    continue;
                }
            }
        }
    }



    // Parse escape sequence.
    uint escapeSequence() {
        uint c;
        int  n;
        int  ndigits;

        c = *p;
        switch (c) {
        case '\'':
        case '"':
        case '?':
        case '\\':
Lconsume:
            p++;
            break;

        case 'a':       c = 7;          goto Lconsume;

        case 'b':       c = 8;          goto Lconsume;

        case 'f':       c = 12;         goto Lconsume;

        case 'n':       c = 10;         goto Lconsume;

        case 'r':       c = 13;         goto Lconsume;

        case 't':       c = 9;          goto Lconsume;

        case 'v':       c = 11;         goto Lconsume;

        case 'u':
            ndigits = 4;
            goto Lhex;

        case 'U':
            ndigits = 8;
            goto Lhex;

        case 'x':
            ndigits = 2;
Lhex:
            p++;
            c = *p;
            if (ishex(c)) {
                uint v;
                n = 0;
                v = 0;
                while (1) {
                    if (isdigit(c)) {
                        c -= '0';
                    }
                    else if (islower(c)) {
                        c -= 'a' - 10;
                    }
                    else {
                        c -= 'A' - 10;
                    }
                    v = v * 16 + c;
                    c = *++p;
                    if (++n == ndigits) {
                        break;
                    }
                    if (!ishex(c)) {
                        error("escape hex sequence has %d hex digits instead of %d", n, ndigits);
                        break;
                    }
                }
//!				    if( ndigits != 2 && !utf_isValidDchar(v))
//!						error("invalid UTF character \\U%08x", v);
                c = v;
            }
            else {
                error("undefined escape hex sequence \\%s\n", c);
            }
            break;

        case '&':                                       // named character entity
            for (ubyte *idstart = ++p; 1; p++) {
                switch (*p) {
                case ';':
                    //!!!
                    /+
                     * c = HtmlNamedEntity(idstart, p - idstart);
                     * if( c == ~0 )
                     * {
                     *      error("unnamed character entity &%.*s;", p - idstart, idstart);
                     *      c = ' ';
                     * }
                     *
                     * p++;
                     +/
                    break;

                default:
                    if (isalpha(*p) || (p != idstart + 1 && isdigit(*p))) {
                        continue;
                    }
                    error("unterminated named entity");
                    break;
                }
                break;
            }
            break;

        case 0:
        case 0x1a:                                      // end of file
            c = '\\';
            break;

        default:
            if (isoctal(c)) {
                ubyte v;
                n = 0;
                do {
                    v = v * 8 + (c - '0');
                    c = *++p;
                } while (++n < 3 && isoctal(c));
                c = v;
            }
            else {
                error("undefined escape sequence \\%s\n", c);
            }
            break;
        }
        return(c);
    }

    /**************************************
     */

    TOK wysiwygStringConstant(Token *t, int tc) {
        uint c;
        Loc  start = loc;

        p++;
        stringbuffer.offset = 0;
        while (1) {
            c = *p++;
            switch (c) {
            case '\n':
                loc.linnum++;
                break;

            case '\r':
                if (*p == '\n') {
                    continue;                           // ignore
                }
                c = '\n';                               // treat EndOfLine as \n character
                loc.linnum++;
                break;

            case 0:
            case 0x1a:
                error("unterminated string constant starting at %s", start.toChars());
                t.ustring = "";
                t.postfix = 0;
                return(TOK.TOKstring);

            case '"':
            case '`':
                if (c == tc) {
//				    t.len = stringbuffer.offset;
                    stringbuffer.write(cast(byte)0);
                    t.ustring = stringbuffer.toString;
//				    t.ustring = (ubyte *)mem.malloc(stringbuffer.offset);
//				    memcpy(t.ustring, stringbuffer.data, stringbuffer.offset);
                    stringPostfix(t);
                    return(TOK.TOKstring);
                }
                break;

            default:
                if (c & 0x80) {
                    p--;
                    uint u = decodeUTF();
                    p++;
                    if (u == PS || u == LS) {
                        loc.linnum++;
                    }
                    stringbuffer.write(u);
                    continue;
                }
                break;
            }
            stringbuffer.write(c);
        }
    }

    /**************************************
     * Lex hex strings:
     *	x"0A ae 34FE BD"
     */

    TOK hexStringConstant(Token *t) {
        uint c;
        Loc  start = loc;
        uint n     = 0;
        uint v;

        p++;
        stringbuffer.offset = 0;
        while (1) {
            c = *p++;
            switch (c) {
            case ' ':
            case '\t':
            case '\v':
            case '\f':
                continue;                                               // skip white space

            case '\r':
                if (*p == '\n') {
                    continue;                                           // ignore
                }

            // Treat isolated '\r' as if it were a '\n'
            case '\n':
                loc.linnum++;
                continue;

            case 0:
            case 0x1a:
                error("unterminated string constant starting at %s", start.toChars());
                t.ustring = "";
                t.postfix = 0;
                return(TOK.TOKstring);

            case '"':
                if (n & 1) {
                    error("odd number (%d) of hex characters in hex string", n);
                    stringbuffer.write(v);
                }
//				t.len = stringbuffer.offset;
//				stringbuffer.write(cast(byte)0);
                t.ustring = stringbuffer.toString;
//				t.ustring = (ubyte *)mem.malloc(stringbuffer.offset);
//				memcpy(t.ustring, stringbuffer.data, stringbuffer.offset);
                stringPostfix(t);
                return(TOK.TOKstring);

            default:
                if (c >= '0' && c <= '9') {
                    c -= '0';
                }
                else if (c >= 'a' && c <= 'f') {
                    c -= 'a' - 10;
                }
                else if (c >= 'A' && c <= 'F') {
                    c -= 'A' - 10;
                }
                else if (c & 0x80) {
                    p--;
                    uint u = decodeUTF();
                    p++;
                    if (u == PS || u == LS) {
                        loc.linnum++;
                    }
                    else {
                        error("non-hex character \\u%x", u);
                    }
                }
                else {
                    error("non-hex character '%s'", c);
                }
                if (n & 1) {
                    v = (v << 4) | c;
                    stringbuffer.write(v);
                }
                else {
                    v = c;
                }
                n++;
                break;
            }
        }
    }

    /**************************************
     */

    TOK escapeStringConstant(Token *t, int wide) {
        uint c;
        Loc  start = loc;

        p++;
        stringbuffer.offset = 0;
        //    debug writefln( "escape string constant: %s", std.string.toString( cast(char*)p ) );
        while (1) {
            c = *p++;
            switch (c) {
            case '\\':
                switch (*p) {
                case 'u':
                case 'U':
                case '&':
                    c = escapeSequence();
                    stringbuffer.write(c);
                    continue;

                default:
                    c = escapeSequence();
                    break;
                }
                break;

            case '\n':
                loc.linnum++;
                break;

            case '\r':
                if (*p == '\n') {
                    continue;                           // ignore
                }
                c = '\n';                               // treat EndOfLine as \n character
                loc.linnum++;
                break;

            case '"':
//			        writefln( "end of string: ", stringbuffer.toString );
                t.ustring = stringbuffer.toString().dup;
                //				t.len = stringbuffer.offset;
                //				stringbuffer.write(cast(byte)0);
                //				t.ustring = (ubyte *)mem.malloc(stringbuffer.offset);
                //				memcpy(t.ustring, stringbuffer.data, stringbuffer.offset);
                stringPostfix(t);

                return(TOK.TOKstring);

            case 0:
            case 0x1a:
                p--;
                error("unterminated string constant starting at %s", start.toChars());
                t.ustring = "";
//					t.len = 0;
                t.postfix = 0;
                return(TOK.TOKstring);

            default:
                if (c & 0x80) {
                    p--;
                    c = decodeUTF();
                    if (c == LS || c == PS) {
                        c = '\n';
                        loc.linnum++;
                    }
                    p++;
                    stringbuffer.write(cast(char)c);
                    continue;
                }
                break;
            }
            stringbuffer.write(cast(char)c);
//			writefln( stringbuffer.toString );
        }
    }

    //**************************************
    TOK charConstant(Token *t, int wide) {
        uint c;
        TOK  tk = TOK.TOKcharv;

        //printf("Lexer.charConstant\n");
        p++;
        c = *p++;
        switch (c) {
        case '\\':
            switch (*p) {
            case 'u':
                t.uns64value = escapeSequence();
                tk           = TOK.TOKwcharv;
                break;

            case 'U':
            case '&':
                t.uns64value = escapeSequence();
                tk           = TOK.TOKdcharv;
                break;

            default:
                t.uns64value = escapeSequence();
                break;
            }
            break;

        case '\n':
L1:
            loc.linnum++;

        case '\r':
        case 0:
        case 0x1a:
        case '\'':
            error("unterminated character constant");
            return(tk);

        default:
            if (c & 0x80) {
                p--;
                c = decodeUTF();
                p++;
                if (c == LS || c == PS) {
                    goto L1;
                }
                if (c < 0xd800 || (c >= 0xe000 && c < 0xfffe)) {
                    tk = TOK.TOKwcharv;
                }
                else {
                    tk = TOK.TOKdcharv;
                }
            }
            t.uns64value = c;
            break;
        }

        if (*p != '\'') {
            error("unterminated character constant");
            return(tk);
        }
        p++;
        return(tk);
    }

    // Get postfix of string literal.
    void stringPostfix(Token *t) {
        switch (*p) {
        case 'c':
        case 'w':
        case 'd':
            t.postfix = *p;
            p++;
            break;

        default:
            t.postfix = 0;
            break;
        }
    }

    /***************************************
     * Read \u or \U unicode sequence
     * Input:
     *	u	'u' or 'U'
     */
    /*
     * uint Wchar(uint u)
     * {
     *  uint value;
     *  uint n;
     *  ubyte c;
     *  uint nchars;
     *
     *  nchars = (u == 'U') ? 8 : 4;
     *  value = 0;
     *  for (n = 0; 1; n++)
     *  {
     * ++p;
     *              if( n == nchars)
     *                  break;
     *              c = *p;
     *              if( !ishex(c))
     *              {
     *                      error("\\%s sequence must be followed by %d hex characters", u, nchars);
     *                  break;
     *              }
     *              if( isdigit(c))
     *                  c -= '0';
     *              else if( islower(c))
     *                  c -= 'a' - 10;
     *              else
     *                  c -= 'A' - 10;
     *              value <<= 4;
     *              value |= c;
     *  }
     *  return value;
     * }
     */

    /**************************************
     * Read in a number.
     * If it's an integer, store it in tok.TKutok.Vlong.
     *	integers can be decimal, octal or hex
     *	Handle the suffixes U, UL, LU, L, etc.
     * If it's double, store it in tok.TKutok.Vdouble.
     * Returns:
     *	TKnum
     *	TKdouble,...
     */

    TOK number(Token *t) {
        //debug writefln("Lexer.number()");
        // We use a state machine to collect numbers
        enum STATE {
            STATE_initial,
            STATE_0,
            STATE_decimal,
            STATE_octal,
            STATE_octale,
            STATE_hex,
            STATE_binary,
            STATE_hex0,
            STATE_binary0,
            STATE_hexh,
            STATE_error
        }

        enum FLAGS {
            FLAGS_decimal  = 1,                         // decimal
            FLAGS_unsigned = 2,                         // u or U suffix
            FLAGS_long     = 4,                         // l or L suffix
        }
        FLAGS flags = FLAGS.FLAGS_decimal;

        int   i;
        TOK   result;
        int   base;

        stringbuffer.offset = 0;
//		stringbuffer.data = null;
        STATE state   = STATE.STATE_initial;
        ubyte * start = p;

        TOK _isreal() {
            p = start;
            return(inreal(t));
        }

        while (true) {
            char c = cast(char)*p;
            switch (state) {
            case STATE.STATE_initial:                           // opening state
                if (c == '0') {
                    state = STATE.STATE_0;
                }
                else {
                    state = STATE.STATE_decimal;
                }
                break;

            case STATE.STATE_0:
                flags = cast(FLAGS)(flags & ~FLAGS.FLAGS_decimal);
                switch (c) {
                //	#if ZEROH
//					    case 'H':			// 0h
//					    case 'h':
//							goto hexh;
                //	#endif
                case 'X':
                case 'x':
                    state = STATE.STATE_hex0;
                    break;

                case '.':
                    if (p[1] == '.') {                                          // .. is a separate token
                        goto done;
                    }

                case 'i':
                case 'f':
                case 'F':
                    goto _Real;

                //	#if ZEROH
//					    case 'E':
//					    case 'e':
//							goto case_hex;
                //	#endif
                case 'B':
                case 'b':
                    state = STATE.STATE_binary0;
                    break;

                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                    state = STATE.STATE_octal;
                    break;

                //	#if ZEROH
//					    case '8': case '9': case 'A':
//					    case 'C': case 'D': case 'F':
//					    case 'a': case 'c': case 'd': case 'f':
//						    case_hex:
//							state = STATE.STATE_hexh;
//							break;
                //	#endif
                case '_':
                    state = STATE.STATE_octal;
                    p++;
                    continue;

                default:
                    goto done;
                }
                break;

            case STATE.STATE_decimal:                           // reading decimal number

                // if its not a digit - decimal complete or not a decimal
                if (!isdigit(c)) {
//						debug writefln( "\tnon-digit( %s )", c );
                    //	#if ZEROH
//					    if( ishex(c) || c == 'H' || c == 'h' )
//							goto hexh;
                    //	#endif
                    //! wtf ?
                    // ignore embedded _
                    if (c == '_') {
                        p++;
                        continue;
                    }

                    // check decimal point - make real
                    if (c == '.' && p[1] != '.') {
                        goto _Real;
                    }

                    // check for mantra - make real
                    if (c == 'i' || c == 'f' || c == 'F' || c == 'e' || c == 'E') {
_Real:                  // It's a real number. Back up and rescan as a real
                        p = start;
                        return(inreal(t));
                    }

                    goto done;
                }
                break;

            case STATE.STATE_hex0:                              // reading hex number
            case STATE.STATE_hex:
                if (!ishex(c)) {
                    if (c == '_') {                                     // ignore embedded _
                        p++;
                        continue;
                    }
                    if (c == '.' && p[1] != '.') {
                        goto _Real;
                    }
                    if (c == 'P' || c == 'p' || c == 'i') {
                        goto _Real;
                    }
                    if (state == STATE.STATE_hex0) {
                        error("Hex digit expected, not '%s'", c);
                    }
                    goto done;
                }
                state = STATE.STATE_hex;
                break;

            //	#if ZEROH
//				    hexh:
//						state = STATE.STATE_hexh;
//
//				    case STATE.STATE_hexh:		// parse numbers like 0FFh
//						if( !ishex(c))
//						{
//						    if( c == 'H' || c == 'h')
//						    {
//								p++;
//								base = 16;
//								goto done;
//						    }
//						    else
//						    {
//								// Check for something like 1E3 or 0E24
//								if( memchr(stringbuffer.data.ptr, 'E', stringbuffer.offset) || memchr( stringbuffer.data.ptr, 'e', stringbuffer.offset))
//								    goto _Real;
//								error("Hex digit expected, not '%s'", c);
//								goto done;
//						    }
//						}
//						break;
            //		#endif

            case STATE.STATE_octal:                                     // reading octal number
            case STATE.STATE_octale:                                    // reading octal number with non-octal digits
                if (!isoctal(c)) {
//				#if ZEROH
//						    if( ishex(c) || c == 'H' || c == 'h' )
//								goto hexh;
//				#endif
                    if (c == '_') {                                             // ignore embedded _
                        p++;
                        continue;
                    }
                    if (c == '.' && p[1] != '.') {
                        goto _Real;
                    }
                    if (c == 'i') {
                        goto _Real;
                    }
                    if (isdigit(c)) {
                        state = STATE.STATE_octale;
                    }
                    else {
                        goto done;
                    }
                }
                break;

            case STATE.STATE_binary0:                                   // starting binary number
            case STATE.STATE_binary:                                    // reading binary number
                if (c != '0' && c != '1') {
                    //			#if ZEROH
//						    if( ishex(c) || c == 'H' || c == 'h' )
//								goto hexh;
                    //			#endif
                    if (c == '_') {                                             // ignore embedded _
                        p++;
                        continue;
                    }
                    if (state == STATE.STATE_binary0) {
                        error("binary digit expected");
                        state = STATE.STATE_error;
                        break;
                    }
                    else {
                        goto done;
                    }
                }
                state = STATE.STATE_binary;
                break;

            case STATE.STATE_error:                                     // for error recovery
                if (!isdigit(c)) {                                      // scan until non-digit
                    goto done;
                }
                break;

            default:
                assert(0);
            }
            stringbuffer.write(cast(char)c);
            p++;
        }
done:
        stringbuffer.write(cast(char)0);                // terminate string

//		debug writefln( "\tdigit complete( %s )", stringbuffer.toString );

        if (state == STATE.STATE_octale) {
            error("Octal digit expected");
        }

        uinteger_t n;                           // unsigned >=64 bit integer type

        if (stringbuffer.offset == 2 && (state == STATE.STATE_decimal || state == STATE.STATE_0)) {
            n = stringbuffer.data[0] - '0';
        }
        else {
            // Convert string to integer
            char* p = cast(char*)stringbuffer.data.ptr;
            int r   = 10;
            int d;
            if (*p == '0') {
                if (p[1] == 'x' || p[1] == 'X') {
                    // "0x#"
                    p += 2;
                    r  = 16;
                }
                else if (p[1] == 'b' || p[1] == 'B') {
                    // "0b#" - binary
                    p += 2;
                    r  = 2;
                }
                else if (isdigit(p[1])) {
                    p += 1;
                    r  = 8;
                }
            }

            n = 0;

            while (true) {
                if (*p >= '0' && *p <= '9') {
                    d = *p - '0';
                }
                else if (*p >= 'a' && *p <= 'z') {
                    d = *p - 'a' + 10;
                }
                else if (*p >= 'A' && *p <= 'Z') {
                    d = *p - 'A' + 10;
                }
                else {
                    break;
                }

                if (d >= r) {
                    break;
                }

                if (n * r + d < n) {
                    error("integer overflow");
                    break;
                }

                n = n * r + d;
                p++;
            }

            // if n needs more than 64 bits
            if (n.sizeof > 8 && n > 0xffffffffffffffffL) {
                error("integer overflow");
            }
        }

        // Parse trailing 'u', 'U', 'l' or 'L' in any combination
        while (true) {
            ubyte f;
            switch (*p) {
            case 'U':
            case 'u':
                f = FLAGS.FLAGS_unsigned;
                goto L1;

            case 'L':
            case 'l':
                f = FLAGS.FLAGS_long;
L1:
                p++;
                if (flags & f) {
                    error("unrecognized token");
                }
                flags = cast(FLAGS)(flags | f);
                continue;

            default:
                break;
            }
            break;
        }

        switch (flags) {
        case 0:
            /* Octal or Hexadecimal constant.
             * First that fits: int, uint, long, ulong
             */
            if (n & 0x8000000000000000L) {
                result = TOK.TOKuns64v;
            }
            else if (n & 0xffffffff00000000L) {
                result = TOK.TOKint64v;
            }
            else if (n & 0x80000000) {
                result = TOK.TOKuns32v;
            }
            else {
                result = TOK.TOKint32v;
            }
            break;

        case FLAGS.FLAGS_decimal:
            /* First that fits: int, long, long long
             */
            if (n & 0x8000000000000000L) {
                error("signed integer overflow");
                result = TOK.TOKuns64v;
            }
            else if (n & 0xffffffff80000000L) {
                result = TOK.TOKint64v;
            }
            else {
                result = TOK.TOKint32v;
            }
            break;

        case FLAGS.FLAGS_unsigned:
        case FLAGS.FLAGS_decimal | FLAGS.FLAGS_unsigned:
            /* First that fits: uint, ulong
             */
            if (n & 0xffffffff00000000L) {
                result = TOK.TOKuns64v;
            }
            else {
                result = TOK.TOKuns32v;
            }
            break;

        case FLAGS.FLAGS_decimal | FLAGS.FLAGS_long:
            if (n & 0x8000000000000000L) {
                error("signed integer overflow");
                result = TOK.TOKuns64v;
            }
            else {
                result = TOK.TOKint64v;
            }
            break;

        case FLAGS.FLAGS_long:
            if (n & 0x8000000000000000L) {
                result = TOK.TOKuns64v;
            }
            else {
                result = TOK.TOKint64v;
            }
            break;

        case FLAGS.FLAGS_unsigned | FLAGS.FLAGS_long:
        case FLAGS.FLAGS_decimal | FLAGS.FLAGS_unsigned | FLAGS.FLAGS_long:
            result = TOK.TOKuns64v;
            break;

        default:
            debug writefln("%x", flags);
            assert(0);
        }
        t.uns64value = n;
        return(result);
    }

    /**************************************
     * Read in characters, converting them to real.
     * Bugs:
     *	Exponent overflow not detected.
     *	Too much requested precision is not detected.
     */

    TOK inreal(Token *t) {
        int  dblstate;
        uint c;
        char hex;                       // is this a hexadecimal-floating-constant?
        TOK  result;

        //printf("Lexer.inreal()\n");
        stringbuffer.offset = 0;
        dblstate            = 0;
        hex                 = 0;
Lnext:
        while (1) {
            // Get next char from input
            c = *p++;
            //printf("dblstate = %d, c = '%s'\n", dblstate, c);
            while (1) {
                switch (dblstate) {
                case 0:                                 // opening state
                    if (c == '0') {
                        dblstate = 9;
                    }
                    else if (c == '.') {
                        dblstate = 3;
                    }
                    else {
                        dblstate = 1;
                    }
                    break;

                case 9:
                    dblstate = 1;
                    if (c == 'X' || c == 'x') {
                        hex++;
                        break;
                    }

                case 1:                                 // digits to left of .
                case 3:                                 // digits to right of .
                case 7:                                 // continuing exponent digits
                    if (!isdigit(c) && !(hex && isxdigit(c))) {
                        if (c == '_') {
                            goto Lnext;                         // ignore embedded '_'
                        }
                        dblstate++;
                        continue;
                    }
                    break;

                case 2:                                 // no more digits to left of .
                    if (c == '.') {
                        dblstate++;
                        break;
                    }

                case 4:                                 // no more digits to right of .
                    if ((c == 'E' || c == 'e') || hex && (c == 'P' || c == 'p')) {
                        dblstate = 5;
                        hex      = 0;                           // exponent is always decimal
                        break;
                    }
                    if (hex) {
                        error("binary-exponent-part required");
                    }
                    goto done;

                case 5:                                 // looking immediately to right of E
                    dblstate++;
                    if (c == '-' || c == '+') {
                        break;
                    }

                case 6:                                 // 1st exponent digit expected
                    if (!isdigit(c)) {
                        error("exponent expected");
                    }
                    dblstate++;
                    break;

                case 8:                                 // past end of exponent digits
                    goto done;
                }
                break;
            }
            stringbuffer.write(c);
        }
done:
        p--;

        stringbuffer.write(cast(byte)0);

//	#if _WIN32 && __DMC__
        char *save = __locale_decpoint;
        __locale_decpoint = ".";
//	#endif
        t.float80value = strtold(cast(char *)stringbuffer.data.ptr, null);
        errno          = 0;
        switch (*p) {
        case 'F':
        case 'f':
            strtof(cast(char *)stringbuffer.data.ptr, null);
            result = TOK.TOKfloat32v;
            p++;
            break;

        default:
            strtod(cast(char *)stringbuffer.data.ptr, null);
            result = TOK.TOKfloat64v;
            break;

        case 'L':
        case 'l':
            result = TOK.TOKfloat80v;
            p++;
            break;
        }
        if (*p == 'i' || *p == 'I') {
            p++;
            switch (result) {
            case TOK.TOKfloat32v:
                result = TOK.TOKimaginary32v;
                break;

            case TOK.TOKfloat64v:
                result = TOK.TOKimaginary64v;
                break;

            case TOK.TOKfloat80v:
                result = TOK.TOKimaginary80v;
                break;
            }
        }
//	#if _WIN32 && __DMC__
        __locale_decpoint = save;
//	#endif
        if (errno == ERANGE) {
            error("number is not representable");
        }
        return(result);
    }




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

        while (1) {
            switch (*p) {
            case 0:
            case 0x1a:
            case '\n':
Lnewline:
                this.loc.linnum = linnum;
                if (filespec.length) {
                    this.loc.filename = filespec;
                }
                return;

            case '\r':
                p++;
                if (*p != '\n') {
                    p--;
                    goto Lnewline;
                }
                continue;

            case ' ':
            case '\t':
            case '\v':
            case '\f':
                p++;
                continue;                                               // skip white space

            case '_':
                if (mod && memcmp(p, cast(char*)"__FILE__", 8) == 0) {
                    p += 8;
//!					    filespec = mem.strdup(loc.filename ? loc.filename : mod.identifier.toChars());
                }
                continue;

            case '"':
                if (filespec) {
                    goto Lerr;
                }
                stringbuffer.offset = 0;
                p++;
                while (1) {
                    uint c;
                    c = *p;
                    switch (c) {
                    case '\n':
                    case '\r':
                    case 0:
                    case 0x1a:
                        goto Lerr;

                    case '"':
                        stringbuffer.write(cast(byte)0);
                        //						    filespec = mem.strdup((char *)stringbuffer.data);
                        filespec = stringbuffer.toString.dup;
                        p++;
                        break;

                    default:
                        if (c & 0x80) {
                            uint u = decodeUTF();
                            if (u == PS || u == LS) {
                                goto Lerr;
                            }
                        }
                        stringbuffer.write(c);
                        p++;
                        continue;
                    }
                    break;
                }
                continue;

            default:
                if (*p & 0x80) {
                    uint u = decodeUTF();
                    if (u == PS || u == LS) {
                        goto Lnewline;
                    }
                }
                goto Lerr;
            }
        }

Lerr:
        errorLoc(loc, "#line integer [\"filespec\"]\\n expected");
    }



    /***************************************************
     * Parse doc comment embedded between t.ptr and p.
     * Remove trailing blanks and tabs from lines.
     * Replace all newlines with \n.
     * Remove leading comment character from each line.
     * Decide if it's a lineComment or a blockComment.
     * Append to previous one for this token.
     */

    void getDocComment(Token *t, uint lineComment) {
        auto OutBuffer buf       = new OutBuffer;
        ubyte          ct        = t.ptr[2];
        ubyte          *q        = t.ptr + 3; // start of comment text
        int            linestart = 0;

        ubyte          *qend = p;

        if (ct == '*' || ct == '+') {
            qend -= 2;
        }

        // Scan over initial row of ****'s or ++++'s or ////'s
        for (; q < qend; q++) {
            if (*q != ct) {
                break;
            }
        }

        // Remove trailing row of ****'s or ++++'s
        if (ct != '/') {
            for (; q < qend; qend--) {
                if (qend[-1] != ct) {
                    break;
                }
            }
        }

        for (; q < qend; q++) {
            ubyte c = *q;

            switch (c) {
            case '*':
            case '+':
                if (linestart && c == ct) {
                    linestart = 0;
                    // Trim preceding whitespace up to preceding \n
                    while (buf.offset && (buf.data[buf.offset - 1] == ' ' || buf.data[buf.offset - 1] == '\t')) {
                        buf.offset--;
                    }
                    continue;
                }
                break;

            case ' ':
            case '\t':
                break;

            case '\r':
                if (q[1] == '\n') {
                    continue;                                   // skip the \r
                }
                goto Lnewline;

            default:
                if (c == 226) {
                    // If LS or PS
                    if (q[1] == 128 &&
                        (q[2] == 168 || q[2] == 169)) {
                        q += 2;
                        goto Lnewline;
                    }
                }
                linestart = 0;
                break;

Lnewline:
                c = '\n';                               // replace all newlines with \n

            case '\n':
                linestart = 1;

                // Trim trailing whitespace
                while (buf.offset && (buf.data[buf.offset - 1] == ' ' || buf.data[buf.offset - 1] == '\t')) {
                    buf.offset--;
                }

                break;
            }
            buf.write(c);
        }

        // Always end with a newline
        if (!buf.offset || buf.data[buf.offset - 1] != '\n') {
            buf.writenl();
        }

        //buf.write(cast(char)0);

        // It's a line comment if the start of the doc comment comes
        // after other non-whitespace on the same line.
//	    ubyte** dc = (lineComment && anyToken)
//				 ? &t.lineComment
//				 : &t.blockComment;

        char[] dc = (lineComment && anyToken) ? t.lineComment : t.blockComment;

        // Combine with previous doc comment, if any
        if (dc.length) {
            dc = combineComments(dc, buf.toString().dup);
        }
        else {
            dc = buf.toString().dup;
        }

//		writefln( dc );

        if (lineComment && anyToken) {
            t.lineComment = dc;
        }
        else {
            t.blockComment = dc;
        }
    }
}

// character maps
static ubyte[256] cmtable;

const int CMoctal  = 0x1;
const int CMhex    = 0x2;
const int CMidchar = 0x4;

ubyte isoctal(ubyte c) {
    return(cmtable[c] & CMoctal);
}
ubyte ishex(ubyte c) {
    return(cmtable[c] & CMhex);
}
ubyte isidchar(ubyte c) {
    return(cmtable[c] & CMidchar);
}

static void cmtable_init() {
    for (uint c = 0; c < cmtable.length; c++) {
        if ('0' <= c && c <= '7') {
            cmtable[c] |= CMoctal;
        }
        if (isdigit(c) || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F')) {
            cmtable[c] |= CMhex;
        }
        if (isalnum(c) || c == '_') {
            cmtable[c] |= CMidchar;
        }
    }
}


/+
 * struct StringValue
 * {
 *  union
 *  {
 *              int intvalue;
 *              void *ptrvalue;
 *              dchar *string;
 *  }
 *
 *  char[] lstring;
 * }
 * #define CASE_BASIC_TYPES
 *      case TOKwchar: case TOKdchar:
 *      case TOKbit: case TOKbool: case TOKchar:
 *      case TOKint8: case TOKuns8:
 *      case TOKint16: case TOKuns16:
 *      case TOKint32: case TOKuns32:
 *      case TOKint64: case TOKuns64:
 *      case TOKfloat32: case TOKfloat64: case TOKfloat80:
 *      case TOKimaginary32: case TOKimaginary64: case TOKimaginary80:
 *      case TOKcomplex32: case TOKcomplex64: case TOKcomplex80:
 *      case TOKvoid:
 *
 * #define CASE_BASIC_TYPES_X(t)					\
 *      case TOKvoid:	 t = Type::tvoid;  goto LabelX;		\
 *      case TOKint8:	 t = Type::tint8;  goto LabelX;		\
 *      case TOKuns8:	 t = Type::tuns8;  goto LabelX;		\
 *      case TOKint16:	 t = Type::tint16; goto LabelX;		\
 *      case TOKuns16:	 t = Type::tuns16; goto LabelX;		\
 *      case TOKint32:	 t = Type::tint32; goto LabelX;		\
 *      case TOKuns32:	 t = Type::tuns32; goto LabelX;		\
 *      case TOKint64:	 t = Type::tint64; goto LabelX;		\
 *      case TOKuns64:	 t = Type::tuns64; goto LabelX;		\
 *      case TOKfloat32: t = Type::tfloat32; goto LabelX;	\
 *      case TOKfloat64: t = Type::tfloat64; goto LabelX;	\
 *      case TOKfloat80: t = Type::tfloat80; goto LabelX;	\
 *      case TOKimaginary32: t = Type::timaginary32; goto LabelX;	\
 *      case TOKimaginary64: t = Type::timaginary64; goto LabelX;	\
 *      case TOKimaginary80: t = Type::timaginary80; goto LabelX;	\
 *      case TOKcomplex32: t = Type::tcomplex32; goto LabelX;	\
 *      case TOKcomplex64: t = Type::tcomplex64; goto LabelX;	\
 *      case TOKcomplex80: t = Type::tcomplex80; goto LabelX;	\
 *      case TOKbit:	 t = Type::tbit;     goto LabelX;	\
 *      case TOKchar:	 t = Type::tchar;    goto LabelX;	\
 *      case TOKwchar:	 t = Type::twchar; goto LabelX;	\
 *      case TOKdchar:	 t = Type::tdchar; goto LabelX;	\
 *      LabelX
 +/
