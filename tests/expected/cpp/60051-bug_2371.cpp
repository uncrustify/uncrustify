class CMyClass
{
	CMyClass( int a = 0, int b = 0 );
};

class CMyClass2
{
	CMyClass2( int a = 0, int b = 0 );
	CMyClass2( int a = 0 );
};

class CMyClass3
{
	CMyClass3( int a, int b = 0 );
	CMyClass3( int a        = 0 );
};

class CMyClass4
{
	CMyClass4( int a    = 0, int b    = 0 );
	CMyClass4( short aa = 0, char * p = 0 );
};

class CMyClass5
{
	CMyClass5()         = default;
	CMyClass5( int a    = 0, int b    = 0 );
	CMyClass5( short aa = 0, char * p = 0 );
};

class CMyClass6
{
	CMyClass6( const CMyClass6& ) = default;
	CMyClass6( int a              = 0, int b    = 0 );
	CMyClass6( short aa           = 0, char * p = 0 );
};

class CMyClass7
{
	virtual void foo( const void* p = nullptr )   = 0;
	CMyClass7( int a                = 0, int b    = 0 );
	CMyClass7( short aa             = 0, char * p = 0 );
};

class CMyClass8
{
	CMyClass8( int a                = 0, int b    = 0 );
	CMyClass8( short aa             = 0, char * p = 0 );
	virtual void foo( const void* p = nullptr )   = 0;
};

class CMyClass9
{
	CMyClass9( int a              = 0, int b    = 0 );
	CMyClass9( short aa           = 0, char * p = 0 );
	virtual void foo( const void* = nullptr )   = 0;
};

class CMyClassA
{
	CMyClassA( int a                      = 0, int b    = 0 );
	CMyClassA( short aa                   = 0, char * p = 0 );
	virtual void foo( const void* /* p */ = nullptr )   = 0;
};

class CMyClassB
{
	CMyClassB( int a                      = 0, int b    = 0 );
	CMyClassB( short aa                   = 0, char * p = 0 );
	virtual void foo( const void* /* p */ = NULL )      = 0;
};

#define UNUSED(x)

class CMyClassC
{
	CMyClassC( int a                        = 0, int b    = 0 );
	CMyClassC( short aa                     = 0, char * p = 0 );
	virtual void foo( const void* UNUSED(p) = NULL )      = 0;
};

class CMyClassD
{
	CMyClassD( int a                      = 0, int b    = 0 );
	CMyClassD( short aa                   = 0, char * p = 0 );
	virtual void foo( const std::string s = "" )        = 0;
};

class CMyClassE
{
	CMyClassE( int a                      = 0, int b        = 0 );
	CMyClassE( short aa                   = 0, char * p     = 0 );
	virtual void foo( const std::string s = std::string() ) = 0;
};

class CMyClassF
{
	CMyClassF( int a                   = 0, int b    = 0 );
	CMyClassF( short aa                = 0, char * p = 0 );
	virtual void foo( const CString& s = _T( "" ) )  = 0;
};
