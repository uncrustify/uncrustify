/**
*  Emscriptens interface for bound std::vectors
*/
interface EmscriptenVector< T >
{
    get( i : number ) : T
    push_back( elem : T );
    resize( size : number, elem : T );
    size() : number;
    get() : T;
    set( elem : T );
//TODO:
//    isAliasOf();
//    clone();
//    delete();
//    isDeleted() : boolean;
//    deleteLater();
}

/**
*  Emscriptens interface for bound enum types
*/
interface EmscriptenEnumType
{
    //! returns list with value objects of an enum
    values() : EmscriptenVector<EmscriptenEnumTypeObject>;
}

/**
*  Emscriptens interface for bound enum type value objects
*/
interface EmscriptenEnumTypeObject
{
    //! return value of an enum value object
    value : number;
}

declare namespace LibUncrustify
{
    // <editor-fold desc="enums">

    // Example how to iterate below options : forin iterate Options,
    // skip 'values' key, [ s : Options_STRING ] : EmscriptenEnumTypeObject;

    // region enum bindings
    export interface OptionTypeValue extends EmscriptenEnumTypeObject {}
    export interface OptionType extends EmscriptenEnumType
    {
        BOOL : OptionTypeValue;
        IARF : OptionTypeValue;
        LINEEND : OptionTypeValue;
        TOKENPOS : OptionTypeValue;
        NUM : OptionTypeValue;
        UNUM : OptionTypeValue;
        STRING : OptionTypeValue;
    }

    export interface IARFValue extends EmscriptenEnumTypeObject {}
    export interface IARF extends EmscriptenEnumType
    {
        IGNORE : IARFValue;
        ADD : IARFValue;
        REMOVE : IARFValue;
        FORCE : IARFValue;
    }

    export interface LineEndValue extends EmscriptenEnumTypeObject {}
    export interface LineEnd extends EmscriptenEnumType
    {
        LF : LineEndValue;
        CRLF : LineEndValue;
        CR : LineEndValue;
        AUTO : LineEndValue;
    }

    export interface TokenPosValue extends EmscriptenEnumTypeObject {}
    export interface TokenPos extends EmscriptenEnumType
    {
        IGNORE : TokenPosValue;
        BREAK : TokenPosValue;
        FORCE : TokenPosValue;
        LEAD : TokenPosValue;
        TRAIL : TokenPosValue;
        JOIN : TokenPosValue;
        LEAD_BREAK : TokenPosValue;
        LEAD_FORCE : TokenPosValue;
        TRAIL_BREAK : TokenPosValue;
        TRAIL_FORCE : TokenPosValue;
    }

    export interface LogTypeValue extends EmscriptenEnumTypeObject {}
    export interface LogType extends EmscriptenEnumType
    {
        SYS : LogTypeValue;
        ERR : LogTypeValue;
        WARN : LogTypeValue;
        NOTE : LogTypeValue;
        INFO : LogTypeValue;
        DATA : LogTypeValue;
        FILELIST : LogTypeValue;
        LINEENDS : LogTypeValue;
        CASTS : LogTypeValue;
        ALBR : LogTypeValue;
        ALTD : LogTypeValue;
        ALPP : LogTypeValue;
        ALPROTO : LogTypeValue;
        ALNLC : LogTypeValue;
        ALTC : LogTypeValue;
        ALADD : LogTypeValue;
        ALASS : LogTypeValue;
        FVD : LogTypeValue;
        FVD2 : LogTypeValue;
        INDENT : LogTypeValue;
        INDENT2 : LogTypeValue;
        INDPSE : LogTypeValue;
        INDPC : LogTypeValue;
        NEWLINE : LogTypeValue;
        PF : LogTypeValue;
        STMT : LogTypeValue;
        TOK : LogTypeValue;
        ALRC : LogTypeValue;
        CMTIND : LogTypeValue;
        INDLINE : LogTypeValue;
        SIB : LogTypeValue;
        RETURN : LogTypeValue;
        BRDEL : LogTypeValue;
        FCN : LogTypeValue;
        FCNP : LogTypeValue;
        PCU : LogTypeValue;
        DYNKW : LogTypeValue;
        OUTIND : LogTypeValue;
        BCSAFTER : LogTypeValue;
        BCSPOP : LogTypeValue;
        BCSPUSH : LogTypeValue;
        BCSSWAP : LogTypeValue;
        FTOR : LogTypeValue;
        AS : LogTypeValue;
        PPIS : LogTypeValue;
        TYPEDEF : LogTypeValue;
        VARDEF : LogTypeValue;
        DEFVAL : LogTypeValue;
        PVSEMI : LogTypeValue;
        PFUNC : LogTypeValue;
        SPLIT : LogTypeValue;
        FTYPE : LogTypeValue;
        TEMPL : LogTypeValue;
        PARADD : LogTypeValue;
        PARADD2 : LogTypeValue;
        BLANKD : LogTypeValue;
        TEMPFUNC : LogTypeValue;
        SCANSEMI : LogTypeValue;
        DELSEMI : LogTypeValue;
        FPARAM : LogTypeValue;
        NL1LINE : LogTypeValue;
        PFCHK : LogTypeValue;
        AVDB : LogTypeValue;
        SORT : LogTypeValue;
        SPACE : LogTypeValue;
        ALIGN : LogTypeValue;
        ALAGAIN : LogTypeValue;
        OPERATOR : LogTypeValue;
        ASFCP : LogTypeValue;
        INDLINED : LogTypeValue;
        BCTRL : LogTypeValue;
        RMRETURN : LogTypeValue;
        PPIF : LogTypeValue;
        MCB : LogTypeValue;
        BRCH : LogTypeValue;
        FCNR : LogTypeValue;
        OCCLASS : LogTypeValue;
        OCMSG : LogTypeValue;
        BLANK : LogTypeValue;
        OBJCWORD : LogTypeValue;
        CHANGE : LogTypeValue;
        CONTTEXT : LogTypeValue;
        ANNOT : LogTypeValue;
        OCBLK : LogTypeValue;
        FLPAREN : LogTypeValue;
        OCMSGD : LogTypeValue;
        INDENTAG : LogTypeValue;
        NFD : LogTypeValue;
        JDBI : LogTypeValue;
        SETPAR : LogTypeValue;
        SETTYP : LogTypeValue;
        SETFLG : LogTypeValue;
        NLFUNCT : LogTypeValue;
        CHUNK : LogTypeValue;
        GUY98 : LogTypeValue;
        GUY : LogTypeValue;
    }

    export interface TokenTypeValue extends EmscriptenEnumTypeObject {}
    export interface TokenType extends EmscriptenEnumType
    {
        NONE : TokenTypeValue;
        EOF : TokenTypeValue;
        UNKNOWN : TokenTypeValue;
        JUNK : TokenTypeValue;
        WHITESPACE : TokenTypeValue;
        SPACE : TokenTypeValue;
        NEWLINE : TokenTypeValue;
        NL_CONT : TokenTypeValue;
        COMMENT_CPP : TokenTypeValue;
        COMMENT : TokenTypeValue;
        COMMENT_MULTI : TokenTypeValue;
        COMMENT_EMBED : TokenTypeValue;
        COMMENT_START : TokenTypeValue;
        COMMENT_END : TokenTypeValue;
        COMMENT_WHOLE : TokenTypeValue;
        COMMENT_ENDIF : TokenTypeValue;
        IGNORED : TokenTypeValue;
        WORD : TokenTypeValue;
        NUMBER : TokenTypeValue;
        NUMBER_FP : TokenTypeValue;
        STRING : TokenTypeValue;
        STRING_MULTI : TokenTypeValue;
        IF : TokenTypeValue;
        ELSE : TokenTypeValue;
        ELSEIF : TokenTypeValue;
        FOR : TokenTypeValue;
        WHILE : TokenTypeValue;
        WHILE_OF_DO : TokenTypeValue;
        SWITCH : TokenTypeValue;
        CASE : TokenTypeValue;
        DO : TokenTypeValue;
        SYNCHRONIZED : TokenTypeValue;
        VOLATILE : TokenTypeValue;
        TYPEDEF : TokenTypeValue;
        STRUCT : TokenTypeValue;
        ENUM : TokenTypeValue;
        ENUM_CLASS : TokenTypeValue;
        SIZEOF : TokenTypeValue;
        DECLTYPE : TokenTypeValue;
        RETURN : TokenTypeValue;
        BREAK : TokenTypeValue;
        UNION : TokenTypeValue;
        GOTO : TokenTypeValue;
        CONTINUE : TokenTypeValue;
        C_CAST : TokenTypeValue;
        CPP_CAST : TokenTypeValue;
        D_CAST : TokenTypeValue;
        TYPE_CAST : TokenTypeValue;
        TYPENAME : TokenTypeValue;
        TEMPLATE : TokenTypeValue;
        WHERE_SPEC : TokenTypeValue;
        ASSIGN : TokenTypeValue;
        ASSIGN_NL : TokenTypeValue;
        SASSIGN : TokenTypeValue;
        ASSIGN_DEFAULT_ARG : TokenTypeValue;
        ASSIGN_FUNC_PROTO : TokenTypeValue;
        COMPARE : TokenTypeValue;
        SCOMPARE : TokenTypeValue;
        BOOL : TokenTypeValue;
        SBOOL : TokenTypeValue;
        ARITH : TokenTypeValue;
        SARITH : TokenTypeValue;
        CARET : TokenTypeValue;
        DEREF : TokenTypeValue;
        INCDEC_BEFORE : TokenTypeValue;
        INCDEC_AFTER : TokenTypeValue;
        MEMBER : TokenTypeValue;
        DC_MEMBER : TokenTypeValue;
        C99_MEMBER : TokenTypeValue;
        INV : TokenTypeValue;
        DESTRUCTOR : TokenTypeValue;
        NOT : TokenTypeValue;
        D_TEMPLATE : TokenTypeValue;
        ADDR : TokenTypeValue;
        NEG : TokenTypeValue;
        POS : TokenTypeValue;
        STAR : TokenTypeValue;
        PLUS : TokenTypeValue;
        MINUS : TokenTypeValue;
        AMP : TokenTypeValue;
        BYREF : TokenTypeValue;
        POUND : TokenTypeValue;
        PREPROC : TokenTypeValue;
        PREPROC_INDENT : TokenTypeValue;
        PREPROC_BODY : TokenTypeValue;
        PP : TokenTypeValue;
        ELLIPSIS : TokenTypeValue;
        RANGE : TokenTypeValue;
        NULLCOND : TokenTypeValue;
        SEMICOLON : TokenTypeValue;
        VSEMICOLON : TokenTypeValue;
        COLON : TokenTypeValue;
        ASM_COLON : TokenTypeValue;
        CASE_COLON : TokenTypeValue;
        CLASS_COLON : TokenTypeValue;
        CONSTR_COLON : TokenTypeValue;
        D_ARRAY_COLON : TokenTypeValue;
        COND_COLON : TokenTypeValue;
        WHERE_COLON : TokenTypeValue;
        QUESTION : TokenTypeValue;
        COMMA : TokenTypeValue;
        ASM : TokenTypeValue;
        ATTRIBUTE : TokenTypeValue;
        AUTORELEASEPOOL : TokenTypeValue;
        OC_AVAILABLE : TokenTypeValue;
        OC_AVAILABLE_VALUE : TokenTypeValue;
        CATCH : TokenTypeValue;
        WHEN : TokenTypeValue;
        WHERE : TokenTypeValue;
        CLASS : TokenTypeValue;
        DELETE : TokenTypeValue;
        EXPORT : TokenTypeValue;
        FRIEND : TokenTypeValue;
        NAMESPACE : TokenTypeValue;
        PACKAGE : TokenTypeValue;
        NEW : TokenTypeValue;
        OPERATOR : TokenTypeValue;
        OPERATOR_VAL : TokenTypeValue;
        ASSIGN_OPERATOR : TokenTypeValue;
        ACCESS : TokenTypeValue;
        ACCESS_COLON : TokenTypeValue;
        THROW : TokenTypeValue;
        NOEXCEPT : TokenTypeValue;
        TRY : TokenTypeValue;
        BRACED_INIT_LIST : TokenTypeValue;
        USING : TokenTypeValue;
        USING_STMT : TokenTypeValue;
        USING_ALIAS : TokenTypeValue;
        D_WITH : TokenTypeValue;
        D_MODULE : TokenTypeValue;
        SUPER : TokenTypeValue;
        DELEGATE : TokenTypeValue;
        BODY : TokenTypeValue;
        DEBUG : TokenTypeValue;
        DEBUGGER : TokenTypeValue;
        INVARIANT : TokenTypeValue;
        UNITTEST : TokenTypeValue;
        UNSAFE : TokenTypeValue;
        FINALLY : TokenTypeValue;
        FIXED : TokenTypeValue;
        IMPORT : TokenTypeValue;
        D_SCOPE : TokenTypeValue;
        D_SCOPE_IF : TokenTypeValue;
        LAZY : TokenTypeValue;
        D_MACRO : TokenTypeValue;
        D_VERSION : TokenTypeValue;
        D_VERSION_IF : TokenTypeValue;
        PAREN_OPEN : TokenTypeValue;
        PAREN_CLOSE : TokenTypeValue;
        ANGLE_OPEN : TokenTypeValue;
        ANGLE_CLOSE : TokenTypeValue;
        SPAREN_OPEN : TokenTypeValue;
        SPAREN_CLOSE : TokenTypeValue;
        FPAREN_OPEN : TokenTypeValue;
        FPAREN_CLOSE : TokenTypeValue;
        TPAREN_OPEN : TokenTypeValue;
        TPAREN_CLOSE : TokenTypeValue;
        BRACE_OPEN : TokenTypeValue;
        BRACE_CLOSE : TokenTypeValue;
        VBRACE_OPEN : TokenTypeValue;
        VBRACE_CLOSE : TokenTypeValue;
        SQUARE_OPEN : TokenTypeValue;
        SQUARE_CLOSE : TokenTypeValue;
        TSQUARE : TokenTypeValue;
        MACRO_OPEN : TokenTypeValue;
        MACRO_CLOSE : TokenTypeValue;
        MACRO_ELSE : TokenTypeValue;
        LABEL : TokenTypeValue;
        LABEL_COLON : TokenTypeValue;
        FUNCTION : TokenTypeValue;
        FUNC_CALL : TokenTypeValue;
        FUNC_CALL_USER : TokenTypeValue;
        FUNC_DEF : TokenTypeValue;
        FUNC_TYPE : TokenTypeValue;
        FUNC_VAR : TokenTypeValue;
        FUNC_PROTO : TokenTypeValue;
        FUNC_START : TokenTypeValue;
        FUNC_CLASS_DEF : TokenTypeValue;
        FUNC_CLASS_PROTO : TokenTypeValue;
        FUNC_CTOR_VAR : TokenTypeValue;
        FUNC_WRAP : TokenTypeValue;
        PROTO_WRAP : TokenTypeValue;
        MACRO_FUNC : TokenTypeValue;
        MACRO : TokenTypeValue;
        QUALIFIER : TokenTypeValue;
        EXTERN : TokenTypeValue;
        DECLSPEC : TokenTypeValue;
        ALIGN : TokenTypeValue;
        TYPE : TokenTypeValue;
        PTR_TYPE : TokenTypeValue;
        TYPE_WRAP : TokenTypeValue;
        CPP_LAMBDA : TokenTypeValue;
        CPP_LAMBDA_RET : TokenTypeValue;
        TRAILING_RET : TokenTypeValue;
        BIT_COLON : TokenTypeValue;
        OC_DYNAMIC : TokenTypeValue;
        OC_END : TokenTypeValue;
        OC_IMPL : TokenTypeValue;
        OC_INTF : TokenTypeValue;
        OC_PROTOCOL : TokenTypeValue;
        OC_PROTO_LIST : TokenTypeValue;
        OC_GENERIC_SPEC : TokenTypeValue;
        OC_PROPERTY : TokenTypeValue;
        OC_CLASS : TokenTypeValue;
        OC_CLASS_EXT : TokenTypeValue;
        OC_CATEGORY : TokenTypeValue;
        OC_SCOPE : TokenTypeValue;
        OC_MSG : TokenTypeValue;
        OC_MSG_CLASS : TokenTypeValue;
        OC_MSG_FUNC : TokenTypeValue;
        OC_MSG_NAME : TokenTypeValue;
        OC_MSG_SPEC : TokenTypeValue;
        OC_MSG_DECL : TokenTypeValue;
        OC_RTYPE : TokenTypeValue;
        OC_ATYPE : TokenTypeValue;
        OC_COLON : TokenTypeValue;
        OC_DICT_COLON : TokenTypeValue;
        OC_SEL : TokenTypeValue;
        OC_SEL_NAME : TokenTypeValue;
        OC_BLOCK : TokenTypeValue;
        OC_BLOCK_ARG : TokenTypeValue;
        OC_BLOCK_TYPE : TokenTypeValue;
        OC_BLOCK_EXPR : TokenTypeValue;
        OC_BLOCK_CARET : TokenTypeValue;
        OC_AT : TokenTypeValue;
        OC_PROPERTY_ATTR : TokenTypeValue;
        PP_DEFINE : TokenTypeValue;
        PP_DEFINED : TokenTypeValue;
        PP_INCLUDE : TokenTypeValue;
        PP_IF : TokenTypeValue;
        PP_ELSE : TokenTypeValue;
        PP_ENDIF : TokenTypeValue;
        PP_ASSERT : TokenTypeValue;
        PP_EMIT : TokenTypeValue;
        PP_ENDINPUT : TokenTypeValue;
        PP_ERROR : TokenTypeValue;
        PP_FILE : TokenTypeValue;
        PP_LINE : TokenTypeValue;
        PP_SECTION : TokenTypeValue;
        PP_ASM : TokenTypeValue;
        PP_UNDEF : TokenTypeValue;
        PP_PROPERTY : TokenTypeValue;
        PP_BODYCHUNK : TokenTypeValue;
        PP_PRAGMA : TokenTypeValue;
        PP_REGION : TokenTypeValue;
        PP_ENDREGION : TokenTypeValue;
        PP_REGION_INDENT : TokenTypeValue;
        PP_IF_INDENT : TokenTypeValue;
        PP_IGNORE : TokenTypeValue;
        PP_OTHER : TokenTypeValue;
        CHAR : TokenTypeValue;
        DEFINED : TokenTypeValue;
        FORWARD : TokenTypeValue;
        NATIVE : TokenTypeValue;
        STATE : TokenTypeValue;
        STOCK : TokenTypeValue;
        TAGOF : TokenTypeValue;
        DOT : TokenTypeValue;
        TAG : TokenTypeValue;
        TAG_COLON : TokenTypeValue;
        LOCK : TokenTypeValue;
        AS : TokenTypeValue;
        IN : TokenTypeValue;
        BRACED : TokenTypeValue;
        THIS : TokenTypeValue;
        BASE : TokenTypeValue;
        DEFAULT : TokenTypeValue;
        GETSET : TokenTypeValue;
        GETSET_EMPTY : TokenTypeValue;
        CONCAT : TokenTypeValue;
        CS_SQ_STMT : TokenTypeValue;
        CS_SQ_COLON : TokenTypeValue;
        CS_PROPERTY : TokenTypeValue;
        SQL_EXEC : TokenTypeValue;
        SQL_BEGIN : TokenTypeValue;
        SQL_END : TokenTypeValue;
        SQL_WORD : TokenTypeValue;
        SQL_ASSIGN : TokenTypeValue;
        CONSTRUCT : TokenTypeValue;
        LAMBDA : TokenTypeValue;
        ASSERT : TokenTypeValue;
        ANNOTATION : TokenTypeValue;
        FOR_COLON : TokenTypeValue;
        DOUBLE_BRACE : TokenTypeValue;
        CNG_HASINC : TokenTypeValue;
        CNG_HASINCN : TokenTypeValue;
        Q_EMIT : TokenTypeValue;
        Q_FOREACH : TokenTypeValue;
        Q_FOREVER : TokenTypeValue;
        Q_GADGET : TokenTypeValue;
        Q_OBJECT : TokenTypeValue;
        MODE : TokenTypeValue;
        DI : TokenTypeValue;
        HI : TokenTypeValue;
        QI : TokenTypeValue;
        SI : TokenTypeValue;
        NOTHROW : TokenTypeValue;
        WORD_ : TokenTypeValue;
    }

    export interface LanguageValue extends EmscriptenEnumTypeObject {}
    export interface Language extends EmscriptenEnumType
    {
        C : LanguageValue;
        CPP : LanguageValue;
        D : LanguageValue;
        CS : LanguageValue;
        JAVA : LanguageValue;
        OC : LanguageValue;
        VALA : LanguageValue;
        PAWN : LanguageValue;
        ECMA : LanguageValue;
    }

    // endregion enum bindings
    // </editor-fold>

    export interface GenericOptionPtr
    {
        type(): OptionTypeValue;
        description(): string;
        name(): string;
        possibleValues(): EmscriptenVector<string>;
        defaultStr(): string;
        minStr(): string;
        maxStr(): string;
        isDefault: boolean;
        reset(): void
        set(value: string): boolean;
        value(): string;
    }

    export interface OptionGroupPtr
    {
        description: string
        options: EmscriptenVector<GenericOptionPtr>;
    }

    export interface Uncrustify
    {
        OptionType: OptionType;
        IARF: IARF;
        LineEnd: LineEnd;
        TokenPos: TokenPos;
        LogType: LogType;
        TokenType: TokenType;
        Language: Language;

        //! get groups vector
        get_groups() : EmscriptenVector <OptionGroupPtr>

        //! get options vector
        get_options() : EmscriptenVector <GenericOptionPtr>

        //! destroys the current libUncrustify instance
        destruct() : void;

        //! returns the UNCRUSTIFY_VERSION string
        get_version() : string;

        //! adds a new keyword to Uncrustifys dynamic keyword map (dkwm, keywords.cpp)
        add_keyword( tag : string, type : TokenType ) : void

        //! removes a keyword from Uncrustifys dynamic keyword map (dkwm, keywords.cpp)
        // remove_keyword( tag : string )

        // clears Uncrustifys dynamic keyword map (dkwm, keywords.cpp)
        clear_keywords() : void;

        //! sets all option values to their default values
        reset_options() : void;

        /**
        * resets value of an option to its default
        *
        * @param name:  name of the option
        * @return options enum value of the found option or -1 if option was not found
        */
        option_reset_value( name : string ) : number;

        /**
        * sets value of an option
        *
        * @param name   name of the option
        * @param value  value that is going to be set
        * @return options enum value of the found option or -1 if option was not found
        */
        option_set_value( name : string, value : string ) : number;

        /**
        * returns value of an option
        *
        * @param name   name of the option
        * @return currently set value of the option
        */
        option_get_value( name : string ) : string;

        /**
        * reads option file string, sets the defined options
        *
        * @return returns EXIT_SUCCESS on success
        */
        load_config( cfg : string ) : number;

        /**
        * returns the config file string based on the current configuration
        *
        * @param withDoc  false= without documentation true=with documentation text lines
        * @param only_not_default  false=containing all options true=containing only options with non default values
        * @return returns the config file string based on the current configuration
        */
        show_config( withDoc : boolean, only_not_default : boolean ) : string;

        /**
        * returns the config file string with all options based on the current configuration
        *
        * @param withDoc  false= without documentation true=with documentation text lines
        * @return returns the config file string with all options based on the current configuration
        */
        show_config( withDoc : boolean ) : string;

        /**
        * returns the config file string with all options and without documentation based on the current configuration
        *
        * @return returns the config file string with all options without documentation based on the current configuration
        */
        show_config() : string;

        //! enable or disable logging of a specific LogType
        log_type_enable(type : LogType, value : bool) : void

        /**
        * Show or hide the severity prefix "<1>"
        *
        * @param b true=show  false=hide
        */
        log_type_show_name( b : boolean ) : void;

        //! disables all logging messages
        quiet() : void;

        /**
         * format text
         *
         * @param file file string that is going to be formated
         * @param lang specifies in which language the input file is written (see LangFlag)
         * @param frag [optional] true=fragmented code input
         *                        false=unfragmented code input [default]
         *
         * @return formatted file string
         */
        uncrustify( file : string, lang : LanguageValue, frag : boolean ) : string;
        uncrustify( file : string, lang : LanguageValue ) : string;

        /**
         * generate debug output
         *
         * @param file file string that is going to be formated
         * @param lang specifies in which language the input file is written (see LangFlag)
         * @param frag [optional] true=fragmented code input
         *                        false=unfragmented code input [default]
         *
         * @return debug output string
         */
        debug( file : string, lang : LanguageValue, frag : boolean ) : string;
        debug( file : string, lang : LanguageValue ) : string;
    }

    var Uncrustify : {
        (module?: Object): Uncrustify;
        new (module?: Object): Uncrustify;
    };
}

declare var uncrustify : LibUncrustify.Uncrustify;

declare module "libUncrustify"
{
    export = uncrustify;
}
