a::b c()
{
	mapped_file_source abc((int)CW1A(sTemp));
	mapped_file_source abc((int)::CW2A(sTemp));
	mapped_file_source abc((int)A::CW3A(sTemp));
}

boost::iostreams::mapped_file_source pdf((LPSTR)ATL::CW2A(sTemp));
