#ifndef HEADER_ASN1_H
#define HEADER_ASN1_H

#define V_ASN1_UNIVERSAL		0x00
#define	V_ASN1_APPLICATION		0x40
#define V_ASN1_CONTEXT_SPECIFIC		0x80
#define V_ASN1_PRIVATE			0xc0

#define V_ASN1_CONSTRUCTED		0x20
#define V_ASN1_PRIMITIVE_TAG		0x1f
#define V_ASN1_PRIMATIVE_TAG		0x1f

#define V_ASN1_APP_CHOOSE		-2	/* let the recipient choose */
#define V_ASN1_OTHER			-3	/* used in ASN1_TYPE */
#define V_ASN1_ANY			-4	/* used in ASN1 template code */

#define V_ASN1_NEG			0x100	/* negative flag */

#define V_ASN1_UNDEF			-1
#define V_ASN1_EOC			0
#define V_ASN1_BOOLEAN			1	/**/
#define V_ASN1_INTEGER			2
#define V_ASN1_NEG_INTEGER		(2 | V_ASN1_NEG)
#define V_ASN1_BIT_STRING		3
#define V_ASN1_OCTET_STRING		4
#define V_ASN1_NULL			5
#define V_ASN1_OBJECT			6  /* object identifier */
#define V_ASN1_OBJECT_DESCRIPTOR	7
#define V_ASN1_EXTERNAL			8  /* external / instance of */
#define V_ASN1_REAL			9
#define V_ASN1_ENUMERATED		10
#define V_ASN1_NEG_ENUMERATED		(10 | V_ASN1_NEG)
#define V_ASN1_EMBEDDED_PDV		11
#define V_ASN1_UTF8STRING		12
#define V_ASN1_SEQUENCE			16
#define V_ASN1_SET			17
#define V_ASN1_NUMERICSTRING		18	/**/
#define V_ASN1_PRINTABLESTRING		19
#define V_ASN1_T61STRING		20
#define V_ASN1_TELETEXSTRING		20	/* alias */
#define V_ASN1_VIDEOTEXSTRING		21  /**/
#define V_ASN1_IA5STRING		22
#define V_ASN1_UTCTIME			23
#define V_ASN1_GENERALIZEDTIME		24	/**/
#define V_ASN1_GRAPHICSTRING		25	/**/
#define V_ASN1_ISO64STRING		26	/**/
#define V_ASN1_VISIBLESTRING		26	/* alias */
#define V_ASN1_GENERALSTRING		27	/**/
#define V_ASN1_UNIVERSALSTRING		28	/**/
#define V_ASN1_BMPSTRING		30

/* For use with d2i_ASN1_type_bytes() */
#define B_ASN1_NUMERICSTRING	0x0001
#define B_ASN1_PRINTABLESTRING	0x0002
#define B_ASN1_T61STRING	0x0004
#define B_ASN1_TELETEXSTRING	0x0004
#define B_ASN1_VIDEOTEXSTRING	0x0008
#define B_ASN1_IA5STRING	0x0010
#define B_ASN1_GRAPHICSTRING	0x0020
#define B_ASN1_ISO64STRING	0x0040
#define B_ASN1_VISIBLESTRING	0x0040
#define B_ASN1_GENERALSTRING	0x0080
#define B_ASN1_UNIVERSALSTRING	0x0100
#define B_ASN1_OCTET_STRING	0x0200
#define B_ASN1_BIT_STRING	0x0400
#define B_ASN1_BMPSTRING	0x0800
#define B_ASN1_UNKNOWN		0x1000
#define B_ASN1_UTF8STRING	0x2000
#define B_ASN1_UTCTIME		0x4000
#define B_ASN1_GENERALIZEDTIME	0x8000
#define B_ASN1_SEQUENCE		0x10000

/* For use with ASN1_mbstring_copy() */
#define MBSTRING_FLAG		0x1000
#define MBSTRING_UTF8		(MBSTRING_FLAG)
#define MBSTRING_ASC		(MBSTRING_FLAG|1)
#define MBSTRING_BMP		(MBSTRING_FLAG|2)
#define MBSTRING_UNIV		(MBSTRING_FLAG|4)

#define SMIME_OLDMIME		0x400
#define SMIME_CRLFEOL		0x800
#define SMIME_STREAM		0x1000

struct X509_algor_st;
DECLARE_STACK_OF(X509_ALGOR);

#define DECLARE_ASN1_SET_OF(type) /* filled in by mkstack.pl */
#define IMPLEMENT_ASN1_SET_OF(type) /* nothing, no longer needed */

/* We MUST make sure that, except for constness, asn1_ctx_st and
   asn1_const_ctx are exactly the same.  Fortunately, as soon as
   the old ASN1 parsing macros are gone, we can throw this away
   as well... */
typedef struct asn1_ctx_st
	{
	unsigned char *p;					/* work char pointer */
	int eos;						/* end of sequence read for indefinite encoding */
	int error;	/* error code to use when returning an error */
	int inf;	/* constructed if 0x20, indefinite is 0x21 */
	int tag;						/* tag from last 'get object' */
	int xclass;	/* class from last 'get object' */
	size_t slen;				/* length of last 'get object' */
	unsigned char *max; /* largest value of p allowed */
	unsigned char *q;/* temporary variable */
	unsigned char **pp;/* variable */
	int line;	/* used in error processing */
	} ASN1_CTX;

typedef struct asn1_const_ctx_st
	{
	const unsigned char *p;/* work char pointer */
	int eos;	                               /* end of sequence read for indefinite encoding */
	int error;	/* error code to use when returning an error */
	int inf; /* constructed if 0x20, indefinite is 0x21 */
	int tag;                    /* tag from last 'get object' */
	int xclass;                                       /* class from last 'get object' */
	size_t slen;	/* length of last 'get object' */
	const unsigned char *max; /* largest value of p allowed */
	const unsigned char *q;/* temporary variable */
	const unsigned char **pp;/* variable */
	int line;	/* used in error processing */
	} ASN1_const_CTX;

/* These are used internally in the ASN1_OBJECT to keep track of
 * whether the names and data need to be free()ed */
#define ASN1_OBJECT_FLAG_DYNAMIC	 0x01	/* internal use */
#define ASN1_OBJECT_FLAG_CRITICAL	 0x02	/* critical x509v3 object id */
#define ASN1_OBJECT_FLAG_DYNAMIC_STRINGS 0x04	/* internal use */
#define ASN1_OBJECT_FLAG_DYNAMIC_DATA 	 0x08	/* internal use */
typedef struct asn1_object_st
	{
	const char *sn,*ln;
	int nid;
	size_t length;
	const unsigned char *data;	/* data remains const after init */
	int flags;	/* Should we free this one */
	} ASN1_OBJECT;

#define ASN1_STRING_FLAG_BITS_LEFT 0x08 /* Set if 0x07 has bits left value */
/* This indicates that the ASN1_STRING is not a real value but just a place
 * holder for the location where indefinite length constructed data should
 * be inserted in the memory buffer 
 */
#define ASN1_STRING_FLAG_NDEF 0x010 

/* This flag is used by the CMS code to indicate that a string is not
 * complete and is a place holder for content when it had all been 
 * accessed. The flag will be reset when content has been written to it.
 */

#define ASN1_STRING_FLAG_CONT 0x020 

/* This is the base type that holds just about everything :-) */
typedef struct asn1_string_st
	{
	size_t length;
	int type;
	unsigned char *data;
	/* The value of the following field depends on the type being
	 * held.  It is mostly being used for BIT_STRING so if the
	 * input data has a non-zero 'unused bits' value, it will be
	 * handled correctly */
	long flags;
	} ASN1_STRING;

/* ASN1_ENCODING structure: this is used to save the received
 * encoding of an ASN1 type. This is useful to get round
 * problems with invalid encodings which can break signatures.
 */

typedef struct ASN1_ENCODING_st
	{
	unsigned char *enc;	/* DER encoding */
	size_t len;		/* Length of encoding */
	int modified;		 /* set to 1 if 'enc' is invalid */
	} ASN1_ENCODING;

/* Used with ASN1 LONG type: if a long is set to this it is omitted */
#define ASN1_LONG_UNDEF	0x7fffffffL

#define STABLE_FLAGS_MALLOC	0x01
#define STABLE_NO_MASK		0x02
#define DIRSTRING_TYPE	\
 (B_ASN1_PRINTABLESTRING|B_ASN1_T61STRING|B_ASN1_BMPSTRING|B_ASN1_UTF8STRING)
#define PKCS9STRING_TYPE (DIRSTRING_TYPE|B_ASN1_IA5STRING)

/* Declarations for template structures: for full definitions
 * see asn1t.h
 */
typedef struct ASN1_TEMPLATE_st ASN1_TEMPLATE;
typedef struct ASN1_ITEM_st ASN1_ITEM;
typedef struct ASN1_TLC_st ASN1_TLC;
/* This is just an opaque pointer */
typedef struct ASN1_VALUE_st ASN1_VALUE;

/* Declare ASN1 functions: the implement macro in in asn1t.h */

#define DECLARE_ASN1_FUNCTIONS(type) DECLARE_ASN1_FUNCTIONS_name(type, type)

#define DECLARE_ASN1_ALLOC_FUNCTIONS(type) \
	DECLARE_ASN1_ALLOC_FUNCTIONS_name(type, type)

#define DECLARE_ASN1_FUNCTIONS_name(type, name) \
	DECLARE_ASN1_ALLOC_FUNCTIONS_name(type, name); \
	DECLARE_ASN1_ENCODE_FUNCTIONS(type, name, name)

#define DECLARE_ASN1_FUNCTIONS_fname(type, itname, name) \
	DECLARE_ASN1_ALLOC_FUNCTIONS_name(type, name); \
	DECLARE_ASN1_ENCODE_FUNCTIONS(type, itname, name)

#define	DECLARE_ASN1_ENCODE_FUNCTIONS(type, itname, name) \
	type *d2i_##name(type **a, const unsigned char **in, size_t len); \
	int i2d_##name(const type *a, unsigned char **out); \
	DECLARE_ASN1_ITEM(itname)

#define	DECLARE_ASN1_ENCODE_FUNCTIONS_const(type, name) \
	type *d2i_##name(type **a, const unsigned char **in, size_t len); \
	int i2d_##name(const type *a, unsigned char **out); \
	DECLARE_ASN1_ITEM(name)

#define	DECLARE_ASN1_NDEF_FUNCTION(name) \
	int i2d_##name##_NDEF(const name *a, unsigned char **out)

#define DECLARE_ASN1_FUNCTIONS_const(name) \
	DECLARE_ASN1_ALLOC_FUNCTIONS(name); \
	DECLARE_ASN1_ENCODE_FUNCTIONS_const(name, name)

#define DECLARE_ASN1_ALLOC_FUNCTIONS_name(type, name) \
	type *name##_new(void); \
	void name##_free(type *a)

#define DECLARE_ASN1_PRINT_FUNCTION(stname) \
	DECLARE_ASN1_PRINT_FUNCTION_fname(stname, stname)

#define DECLARE_ASN1_PRINT_FUNCTION_fname(stname, fname) \
	int fname##_print_ctx(BIO *out, const stname *x, int indent, \
					 const ASN1_PCTX *pctx)


/*
 * WARNING WARNING WARNING
 *
 * uncrustify still introduces whitespace in here at some spots, but then
 * one might ask how crazy we want to go regarding ## encumbered parsing?
 * There's always the copout of INDENT-OFF markers for files like these,
 * once you've got them 95% right through uncrustify and that extra 5% 
 * by hand ;-)
 */
#define TYPEDEF_D2I_OF(type) typedef type *d2i_of_##type(type **,const unsigned char **,size_t)
#define TYPEDEF_I2D_OF(type) typedef int i2d_of_##type(type *,unsigned char **)
#define TYPEDEF_I2D_OF_CONST(type) typedef int i2d_of_const_##type(const type *,unsigned char **)    /* [i_a] */
#define TYPEDEF_D2I2D_OF(type) TYPEDEF_D2I_OF(type); TYPEDEF_I2D_OF(type); TYPEDEF_I2D_OF_CONST(type)    /* [i_a] */


/* Macro to include ASN1_ITEM pointer from base type */
#define ASN1_ITEM_ref(iptr) (&(iptr##_it))

#define ASN1_ITEM_rptr(ref) (&(ref##_it))

#define DECLARE_ASN1_ITEM(name) \
	extern const ASN1_ITEM name##_it;


#define ASN1_STRFLGS_RFC2253	(ASN1_STRFLGS_ESC_2253 | \
				ASN1_STRFLGS_ESC_CTRL | \
				ASN1_STRFLGS_ESC_MSB | \
				ASN1_STRFLGS_UTF8_CONVERT | \
				ASN1_STRFLGS_DUMP_UNKNOWN | \
				ASN1_STRFLGS_DUMP_DER)

DECLARE_STACK_OF(ASN1_INTEGER);
DECLARE_ASN1_SET_OF(ASN1_INTEGER);

DECLARE_STACK_OF(ASN1_GENERALSTRING);

typedef STACK_OF(ASN1_TYPE) ASN1_SEQUENCE_ANY;

DECLARE_ASN1_ENCODE_FUNCTIONS_const(ASN1_SEQUENCE_ANY, ASN1_SEQUENCE_ANY);
DECLARE_ASN1_ENCODE_FUNCTIONS_const(ASN1_SEQUENCE_ANY, ASN1_SET_ANY);


#define B_ASN1_DIRECTORYSTRING \
			B_ASN1_PRINTABLESTRING| \
			B_ASN1_TELETEXSTRING|\
			B_ASN1_BMPSTRING|\
			B_ASN1_UNIVERSALSTRING|\
			B_ASN1_UTF8STRING

#define B_ASN1_DISPLAYTEXT \
			B_ASN1_IA5STRING| \
			B_ASN1_VISIBLESTRING| \
			B_ASN1_BMPSTRING|\
			B_ASN1_UTF8STRING

#endif

