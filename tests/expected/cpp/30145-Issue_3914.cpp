void foo()
{
	desc->add_options( )
	( "help,h", "produce help message" )
	( "version,v", "print the version number" )
	( "include-path,I", value< vector<string> >( ), "include path" )
	( "input-file,i", value<string>( ), "input file" );
}
