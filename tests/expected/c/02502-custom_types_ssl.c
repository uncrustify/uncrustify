#ifndef  HEADER_CONF_H
#define HEADER_CONF_H

#ifdef  __cplusplus
extern "C"
#endif
{

	typedef struct
	{
		char *section;
		char *name;
		char *value;
	} CONF_VALUE;

	DECLARE_STACK_OF( CONF_VALUE );
	DECLARE_LHASH_OF( CONF_VALUE );

	struct conf_st;
	struct conf_method_st;
	typedef struct conf_method_st CONF_METHOD;

	int CONF_set_default_method ( CONF_METHOD *meth );
	void CONF_set_nconf ( CONF *conf,LHASH_OF(CONF_VALUE) *hash );
	LHASH_OF(CONF_VALUE) *CONF_load ( LHASH_OF(CONF_VALUE) *conf,const char *file,
		long *eline );
#ifndef OPENSSL_NO_FP_API
	LHASH_OF(CONF_VALUE) *CONF_load_fp ( LHASH_OF(CONF_VALUE) *conf, FILE *fp,
		long *eline );
#endif
	LHASH_OF(CONF_VALUE) *CONF_load_bio ( LHASH_OF(CONF_VALUE) *conf, BIO *bp,long *eline );
	STACK_OF(CONF_VALUE) *CONF_get_section ( LHASH_OF(CONF_VALUE) *conf,
		const char *                                               section );
	char *CONF_get_string ( LHASH_OF(CONF_VALUE) *conf,const char *group,
		const char *name );
	long CONF_get_number ( LHASH_OF(CONF_VALUE) *conf,const char *group,
		const char *name );
	void CONF_free ( LHASH_OF(CONF_VALUE) *conf );
	int CONF_dump_fp ( LHASH_OF(CONF_VALUE) *conf, FILE *out );
	int CONF_dump_bio ( LHASH_OF(CONF_VALUE) *conf, BIO *out );


}


void CONF_set_nconf ( CONF *conf, LHASH_OF(CONF_VALUE) *hash )
{
	if (default_CONF_method == NULL)
		default_CONF_method = NCONF_default();

	default_CONF_method->init( conf );
	conf->data = hash;
}


LHASH_OF(CONF_VALUE) *CONF_load ( LHASH_OF(CONF_VALUE) *conf, const char *file,
	long *eline )
{
	LHASH_OF(CONF_VALUE) *ltmp;
	BIO *in = NULL;

#ifdef OPENSSL_SYS_VMS
	in = BIO_new_file( file, "r" );
#else
	in = BIO_new_file( file, "rb" );
#endif
	if (in == NULL)
	{
		CONFerr( CONF_F_CONF_LOAD,ERR_R_SYS_LIB );
		return NULL;
	}

	return ltmp;
}

#ifndef OPENSSL_NO_FP_API
LHASH_OF(CONF_VALUE) *CONF_load_fp ( LHASH_OF(CONF_VALUE) *conf, FILE *fp,
	long *eline )
{
	BIO *btmp;
	LHASH_OF(CONF_VALUE) *ltmp;
	if(!(btmp = BIO_new_fp( fp, BIO_NOCLOSE ))) {
		CONFerr( CONF_F_CONF_LOAD_FP,ERR_R_BUF_LIB );
		return NULL;
	}
	ltmp = CONF_load_bio( conf, btmp, eline );
	BIO_free( btmp );
	return ltmp;
}
#endif

LHASH_OF(CONF_VALUE) *CONF_load_bio ( LHASH_OF(CONF_VALUE) *conf, BIO *bp,
	long *eline )
{
	CONF ctmp;
	int ret;

	CONF_set_nconf( &ctmp, conf );

	ret = NCONF_load_bio( &ctmp, bp, eline );
	if (ret)
		return ctmp.data;
	return NULL;
}

STACK_OF(CONF_VALUE) *CONF_get_section ( LHASH_OF(CONF_VALUE) *conf,
	const char *                                               section )
{
	if (conf == NULL)
	{
		return NULL;
	}
	else
	{
		CONF ctmp;
		CONF_set_nconf( &ctmp, conf );
		return NCONF_get_section( &ctmp, section );
	}
}

char *CONF_get_string ( LHASH_OF(CONF_VALUE) *conf,const char *group,
	const char *name )
{
	if (conf == NULL)
	{
		return NCONF_get_string( NULL, group, name );
	}
	else
	{
		return NCONF_get_string( &ctmp, group, name );
	}
}

