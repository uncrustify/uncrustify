
void log_fmt( log_sev_t sev, const char *fmt, ... ) __attribute__( ( format( printf, 2, 3 ) ) );

#define LOG_FMT( sev, args... )                           \
	do { if ( log_sev_on( sev ) ) { log_fmt( sev, ## args ); } } while ( 0 )

void foo()
{
	try {}
	catch ( ... ) // <== HERE
	{}
}
