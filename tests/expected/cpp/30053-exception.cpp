 #include <iostream>


void foo()
{
	char *buf;
	try {
		buf = new unsigned char[1024];
		if( buf == 0 )
			throw "Out of memory";
	}
	catch( char * str ) {
		cout << "Exception: " << str << '\n';
	}
}

void bar()
{
	char *buf;

	try
	{
		buf = new unsigned char[1024];
		if( buf == 0 )
			throw "Out of memory";
	}
	catch( char * str )
	{
		cout << "Exception: " << str << '\n';
	}
}


